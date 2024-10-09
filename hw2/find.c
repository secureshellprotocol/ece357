#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define EXIT_OK             0
#define EXIT_FAIL           -1
#define HLINK_LIST_BUFSIZE  64

// Macro to report an error to 'stderr' AND CLOSE AS A FAILURE.
// Include a trailing '%s' in 'message' to also report errno.
#define ERR_CLOSE(message, ...) \
    fprintf(stderr, message"\n", ## __VA_ARGS__, strerror(errno));\
    return EXIT_FAIL;

// Macro to report an error to 'stderr' and continue.
// Include a trailing '%s' in 'message' to also report errno.
#define ERR_CONT(message, ...) \
    fprintf(stderr, message"\n", ## __VA_ARGS__, strerror(errno));

#define USAGE(name) \
    fprintf(stdout, "Usage: %s directory\n", name);

#define st_ind(index) \
    ((index) >> 12)

typedef struct stat_fax_t {
    int inode_count[16];        // Counts no. of inodes of given type.
                                // List is indexed by inode type, eg
                                //  inode_count[S_IFREG] contains no. of file
                                //  inodes.
    int file_size_sum;          // Sum of every encountered regular file's size
    int blk_alloc_sum;          // Sum of disk blocks allocated for all regular
                                // files.
    int dangling_symlink_count; // Counts no. of unresolvable symlinks
    int bad_filename_count;     // Counts no. of file names w/ bad names
                                //  ie: name contains control characters,
                                //  non-printable characters, or non-ASCII chars
    int hlinked_inode_count;    // length of hard_linked_inodes list, tells us
                                // how many inodes we have seen more than once
    ino_t *hlinked_inodes;      // if we come across a (non-directory) inode w/
                                // `st_nlink` > 1, we put it in this list.
} stat_facts;


int hlink_processor(ino_t inode_no, stat_facts *fstree_stats)
{
    for(int i = 0; i < fstree_stats->hlinked_inode_count; i++) {
        if(inode_no == fstree_stats->hlinked_inodes[i]) {
            return EXIT_OK;
        }
    }
    fstree_stats->hlinked_inode_count+=1;

    // check if we need to realloc
    if(fstree_stats->hlinked_inode_count >= 
       (sizeof(fstree_stats->hlinked_inodes) / (sizeof(fstree_stats->hlinked_inodes[0]))))
    {
        ino_t* new_inode_array = 
            reallocarray(fstree_stats->hlinked_inodes,
                         sizeof(ino_t),
                         fstree_stats->hlinked_inode_count * 2);
        if(new_inode_array == NULL) {
            ERR_CLOSE(
                "Failed to reallocate member array `hlinked_inodes` for %d bytes! %s",
                sizeof(ino_t) * fstree_stats->hlinked_inode_count * 2
                );
        }
        fstree_stats->hlinked_inodes = new_inode_array;
    }

    fstree_stats->hlinked_inodes[fstree_stats->hlinked_inode_count - 1] = 
        inode_no;

    return EXIT_OK;
}

int directory_walker(const char *pwd, stat_facts *fstree_stats)
{
    printf("%s", pwd);

    // collect facts on pwd
    struct stat st;
    stat(pwd, &st);
    if((st.st_mode & S_IFMT) != S_IFDIR) {
        ERR_CLOSE("%s is not a directory!", pwd);
    }
    printf("  %d\n", st_ind(S_IFDIR));
    
    DIR *dirp;
    struct dirent *pwd_ent = NULL;
    if((dirp = opendir(pwd)) == NULL) {
        ERR_CLOSE("Could not open %s!", pwd); 
    }

    while((pwd_ent = readdir(dirp)) != NULL) {
        if(strcmp(pwd_ent->d_name, ".") == 0 || 
           strcmp(pwd_ent->d_name, "..") == 0) 
        {
            continue;
        }

        char *filepath = 
            malloc((sizeof(char)) * (strlen(pwd) + strlen(pwd_ent->d_name) + 2));
        strcpy(filepath, pwd);
        strcpy(&filepath[strlen(pwd)], "/");
        strcpy(&filepath[strlen(pwd)+1], pwd_ent->d_name);

        if(lstat(filepath, &st) < 0) {
            ERR_CONT("Could not stat %s! Reason: %s", filepath);
        }

        for(int i = 0; i < strlen(filepath); i++) {
            if(!isascii(filepath[i]) || 
               isblank(filepath[i])  || 
               iscntrl(filepath[i]))
            {
                fstree_stats->bad_filename_count+=1;
                break;
            }
        }

        int current_mode = (st.st_mode & S_IFMT);
        fstree_stats->inode_count[st_ind(current_mode)]+=1;
        if(current_mode == S_IFDIR) {
            directory_walker(filepath, fstree_stats);
            continue;
        }

        printf("%s", filepath);
        printf("  %d\n", st_ind(current_mode));

        if((st.st_nlink > 1)) {
            if(hlink_processor(st.st_ino, fstree_stats) < 0) {
                ERR_CLOSE("Failed to process inode hard-links! %s");
            }
        }

        switch (current_mode) {
        case S_IFREG:
            fstree_stats->file_size_sum+=st.st_size;
            fstree_stats->blk_alloc_sum+=st.st_blocks;
            break;
        case S_IFLNK:
            // does is exist?
            int fd;
            if((fd = open(filepath, O_RDONLY)) < 0) {
                if(errno == ENOENT) {
                    ERR_CONT("Could not stat %s! Reason: %s", filepath);
                    fstree_stats->dangling_symlink_count+=1;
                }
            } else {
                close(fd);
            }
            break;
        default:
            break;
        }
        
        free(filepath);
    }

    return EXIT_OK;
}

int main(int argc, char *argv[]) 
{
    if(argc != 2) {
        USAGE(argv[0]);
        return EXIT_FAIL;
    }

    stat_facts fstree_stats = {
        { 0 },  // inode_count[]
        0,      // file_size_sum
        0,      // blk_alloc_sum
        0,      // dangling_symlink_count
        0,      // bad_filename_count
        0,      // hlinked_inode_count
        NULL    // hlinked_inodes
    };
    
    fstree_stats.inode_count[st_ind(S_IFDIR)]+=1; // Our root inode!
    fstree_stats.hlinked_inodes = malloc(HLINK_LIST_BUFSIZE * sizeof(ino_t));
    if(directory_walker(argv[1], &fstree_stats) < 0) {
        ERR_CLOSE("Directory walk routine failed! Tried to access %s",argv[1]);
    }
    
    printf("Total size of all encountered regular files: %d\n", 
           fstree_stats.file_size_sum);
    printf("Total number of allocated disk blocks: %d\n",
           fstree_stats.blk_alloc_sum);
    printf("Total number of dangling symlinks: %d\n",
           fstree_stats.dangling_symlink_count);
    printf("Total number of (non-directory) inodes w/ more than 1 hard link: %d\n",
           fstree_stats.hlinked_inode_count);
    printf("Total number of pathnames with problematic names: %d\n",
           fstree_stats.bad_filename_count);
    return EXIT_OK;
}


