#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/rbtree.h>
#include <linux/radix-tree.h>
#include <linux/xarray.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static char *int_str;

//Module Description
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NITHEESH PRAKASH");
MODULE_DESCRIPTION("LKP Project 2");
module_param(int_str, charp, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(int_str, "A comma-separated list of integers");


static LIST_HEAD(mylist);
#define hash_bit 10
static DEFINE_HASHTABLE(hash_define,hash_bit);
struct rb_root myroot= RB_ROOT;
RADIX_TREE(radix, GFP_KERNEL);
DEFINE_XARRAY(xarray);


struct entry {
	int val;
	struct list_head list;
};
struct hash_entry{
	int val;
	struct hlist_node hash;
	};
struct rb_entry{
	int val;
	struct rb_node node;
};
struct radix_entry{
	struct rcu_head rcu_head;
	unsigned long index;
	unsigned int val;
};
struct xarray_entry{
	unsigned long index;
	unsigned int val;
};


//STORE VALUE
static int store_value_list(int val)
{
	struct entry *temp;
	temp= kmalloc(sizeof(struct entry),GFP_KERNEL);
	if(!temp)
		return -ENOMEM;
	temp->val=val;
	list_add_tail(&(temp->list),&(mylist));
	return 0;
}

static int store_value_hash(int val)
{
	struct hash_entry *temp;
	temp=kmalloc(sizeof(struct hash_entry),GFP_KERNEL);
	if(!temp)
		return -ENOMEM;
	temp->val=val;
	hash_add(hash_define,&temp->hash,val);
	return 0;
}

static int store_value_rbtree(int val)
{
	struct rb_entry *temp;
	struct rb_node **link = &myroot.rb_node, *parent = NULL;
	struct rb_entry *entry;
	temp=kmalloc(sizeof(struct rb_entry),GFP_KERNEL);
	if(!temp)
		return -ENOMEM;
	temp->val=val;
	while (*link) {
		parent = *link;
		entry=rb_entry(parent,struct rb_entry,node);
		if (temp->val > entry->val)
			link = &(*link)->rb_left;
		else 
			link= &(*link)->rb_right;
		}
	rb_link_node(&temp->node, parent, link);
	rb_insert_color(&temp->node,&myroot);
	return 0;
}

static int store_value_radix(int val)
{
	struct radix_entry *temp=kmalloc(sizeof(struct radix_entry),GFP_KERNEL);
	int err;
	if(!temp)
		return -ENOMEM;
	temp->val=val;
	temp->index=val;
	err=radix_tree_insert(&radix,temp->index,temp);
	return 0;
}

static int store_value_xarray(int val)
{
	struct xarray_entry *temp=kmalloc(sizeof(struct xarray_entry), GFP_KERNEL);
	if(!temp)
		return -ENOMEM;
	temp->val=val;
	temp->index=val;
	xa_store(&xarray, val, (void *) temp,GFP_KERNEL);
	return 0;
}


//ITERATE OVER THE DATATYPE
static void test_linked_list(void)
{
	struct entry *temp;
	printk("\nThe entries of the linked list are :");
	list_for_each_entry(temp,&mylist,list){
		printk(KERN_INFO"\n %d",temp->val);
	}	
}

static void test_hashtable(void)
{
	struct hash_entry *temp;
	int bkt=0;
	printk("\nThe entries of the HASH TABLE are :");
	hash_for_each(hash_define,bkt,temp,hash){
		printk(KERN_INFO"\n %d",temp->val);
	}
}

static void test_rbtree(void)
{
	struct rb_node *node = rb_last(&myroot);
	printk("\n The entries of the rb-tree are");
	while(node)
	{
		struct rb_entry *temp = rb_entry(node, struct rb_entry, node);
		printk(KERN_INFO "\n %d \n",temp->val);
		node = rb_prev(node);
	}
}
	
static void test_radixtree(void)
{
	void **slot;
	struct radix_tree_iter iter;
        struct radix_entry *temp;
	printk("\nThe entries of the radix tree are");
	radix_tree_for_each_slot(slot,&radix,&iter,0){
		temp = radix_tree_deref_slot(slot);
		printk(KERN_INFO"\n %d \n",temp->val);
	}	

}

static void test_xarray(void)
{
	void * temp;
	unsigned long i=0;
	printk("\n The entries of the xarray tree are");
	xa_for_each(&xarray, i,temp){
		printk("\n %d\n",((struct xarray_entry *)temp)->val);
	}
}


//Destroy and free the datastructure
static void destroy_linked_list_and_free(void)
{
	struct entry *temp,*next;
	list_for_each_entry_safe(temp,next,&mylist,list){
		list_del(&temp->list);
		kfree(temp);
	}
}

static void destroy_hashtable(void)
{
	struct hash_entry *temp;
        int bkt=0;
	hash_for_each(hash_define,bkt,temp,hash){
		hash_del(&temp->hash);
		kfree(temp);
	}
}

static void destroy_rbtree(void)
{
	struct rb_node *node = rb_first(&myroot);
	while(node)
		{
			struct rb_entry *temp = rb_entry(node, struct rb_entry, node);
			node = rb_next(node);
			rb_erase(&temp->node,&myroot);
			kfree(temp);
		}
}

static void destroy_radix(void)
{
	void **slot;
	struct radix_tree_iter iter;
        struct radix_entry *temp;
	radix_tree_for_each_slot(slot,&radix,&iter,0){
		temp = radix_tree_deref_slot(slot);
		temp = radix_tree_delete(&radix, temp->index);
		kfree(temp);
	}
}
		
static void destroy_xarray(void)
{
	unsigned long i=1;
	void *temp;
	xa_for_each(&xarray,i,temp){
		xa_erase(&xarray,i);
	}
	xa_destroy(&xarray);
}

//print in the proc node
static int proj2_show(struct seq_file *m, void *v) {
	
	struct entry *temp;
	void * temp4;
	unsigned long i1=0;
	struct rb_node *node = rb_last(&myroot);
	struct hash_entry *temp1;
	int bkt=0;
	void **slot;
	struct radix_tree_iter iter;
        struct radix_entry *temp3;

	//Linked List
	seq_printf(m,"\nLinked List :");
	list_for_each_entry(temp,&mylist,list){
		seq_printf(m," %d,",temp->val);
	}

	//HAsh Table
	seq_printf(m,"\b ");
	seq_printf(m,"\nHash Table : ");
	hash_for_each(hash_define,bkt,temp1,hash){
		seq_printf(m," %d,",temp1->val);
	}

	//RED-BLACK tree		
	seq_printf(m,"\b ");
	seq_printf(m,"\nRed-Black Tree : ");
	while(node)
	{
		struct rb_entry *temp2 = rb_entry(node, struct rb_entry, node);
		seq_printf(m," %d,",temp2->val);
		node = rb_prev(node);
	}

	//Radix Tree
	seq_printf(m,"\b ");
	seq_printf(m,"\nRadix Tree : ");
	radix_tree_for_each_slot(slot,&radix,&iter,0){
		temp3 = radix_tree_deref_slot(slot);
		seq_printf(m," %d,",temp3->val);
	}	
	
	//XArray
	seq_printf(m,"\b ");
	seq_printf(m,"\nXArray tree : ");
	xa_for_each(&xarray, i1,temp4){
		seq_printf(m," %d,",((struct xarray_entry *)temp4)->val);
	}
	seq_printf(m,"\b \n");
	return 0;
}

		

static int parse_params(void)
{
	int val, err = 0;
	char *p, *orig, *params;


	params = kstrdup(int_str, GFP_KERNEL);
	if (!params)
		return -ENOMEM;
	orig = params;

	while ((p = strsep(&params, ",")) != NULL) {
		if (!*p)
			continue;
		err = kstrtoint(p, 0, &val);
		if (err)
			break;

		err = store_value_list(val);
		err = store_value_hash(val);
		err = store_value_rbtree(val);
		err = store_value_radix(val);
		err = store_value_xarray(val);
		if (err)
			break;
	}

	kfree(orig);
	return err;
}

static void run_tests(void)
{
	test_linked_list();
	test_hashtable();
	test_rbtree();
	test_radixtree();
	test_xarray();
}

static void cleanup(void)
{
	printk(KERN_INFO "\nCleaning up...\n");
	destroy_linked_list_and_free();
	destroy_hashtable();
	destroy_rbtree();
	destroy_radix();
	destroy_xarray();
}

static int proj2_open(struct inode *inode, struct  file *file) {
	return single_open(file, proj2_show, NULL);
}

static const struct proc_ops proj2_fops = {
	.proc_open = proj2_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __init ex3_init(void)
{
	int err = 0;

	if (!int_str) {
		printk(KERN_INFO "Missing \'int_str\' parameter, exiting\n");
		return -1;
	}

	err = parse_params();
	if (err)
		goto out;

	run_tests();
	proc_create("proj2", 0, NULL, &proj2_fops);

out:
	return err;
}

static void __exit ex3_exit(void)
{
        remove_proc_entry("proj2", NULL);
	cleanup();
	return;
}

module_init(ex3_init);

module_exit(ex3_exit);
