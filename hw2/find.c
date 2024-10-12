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
#define EXIT_SEEN            1  /* hlink_processor specific */
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

// Prints usage information
#define USAGE(name) \
    fprintf(stdout, "Usage: %s directory\n", name);

// Used to quickly isolate mode bits, used for indexing inode_count under
//  stat_facts.
#define st_ind(index) \
    ((index) >> 12)

// Treewalk statistic struct
typedef struct stat_fax_t {
    long unsigned int inode_count[16];        // Counts no. of inodes of given type.
                                              // List is indexed by inode type, eg
                                              //  inode_count[S_IFREG] contains no. of file
                                              //  inodes.
    long unsigned int file_size_sum;          // Sum of every encountered regular file's size
    long unsigned int blk_alloc_sum;          // Sum of disk blocks allocated for all regular
                                              // files.
    long unsigned int dangling_symlink_count; // Counts no. of unresolvable symlinks
    long unsigned int bad_filename_count;     // Counts no. of file names w/ bad names
                                              //  ie: name contains control characters,
                                              //  non-printable characters, or non-ASCII chars
    long unsigned int hlinked_inode_count;    // length of hard_linked_inodes list, tells us
                                              // how many inodes we have seen more than once
    ino_t *hlinked_inodes;                    // if we come across a (non-directory) inode w/
                                              // `st_nlink` > 1, we put it in this list.
} stat_facts;

// hlink_processor: processes, for a given inode, whether or not we have seen it
//  already. This information is stored in the `hlinked_inodes` array within our
//  statistics structure. This makes no assumptions on how many inodes we are
//  already linked to, just whether or not we have seen it already!
//
// Returns EXIT_OK if we have not seen this inode
// Returns EXIT_SEEN if we have seen this inode
// Returns EXIT_FAIL if we fail to expand `hlinked_inodes` when accomodating a
//  large amount of inodes
int hlink_processor(ino_t inode_no, stat_facts *fstree_stats)
{
    for(int i = 0; i < fstree_stats->hlinked_inode_count; i++) {
        if(inode_no == fstree_stats->hlinked_inodes[i]) {
            return EXIT_SEEN;
        }
    }
    fstree_stats->hlinked_inode_count+=1;

    // check if we need to realloc
    if(fstree_stats->hlinked_inode_count >= 
       (sizeof(fstree_stats->hlinked_inodes) / (sizeof(fstree_stats->hlinked_inodes[0]))))
    {
        ino_t* new_inode_array = 
            // HAK - your machine doesnt like 'reallocarray()', so i translated
            // my call to 'realloc()'
            //reallocarray(fstree_stats->hlinked_inodes,
            //             sizeof(ino_t),
            //             fstree_stats->hlinked_inode_count * 2);
            realloc(fstree_stats->hlinked_inodes,
                    sizeof(ino_t) * fstree_stats->hlinked_inode_count * 2);
        if(new_inode_array == NULL) {
            ERR_CLOSE(
                "Failed to reallocate member array `hlinked_inodes` for %lu bytes! %s",
                sizeof(ino_t) * fstree_stats->hlinked_inode_count * 2
                ); // EXIT_FAIL returned
        }
        fstree_stats->hlinked_inodes = new_inode_array;
    }

    fstree_stats->hlinked_inodes[fstree_stats->hlinked_inode_count - 1] = 
        inode_no;

    return EXIT_OK;
}

// Takes our present working dir, and prepends it to a `fname` of interest
// Sets `*filepath` to a valid pointer on success. 
// `*filepath` is NULL on any failure.
//
// The user is responsible for freeing! We do no garbage collection!
void get_filepath(const char *pwd, const char *fname, char **filepath){
    *filepath = NULL; // None at all! 

    char *filepath_tmp = 
        malloc((sizeof(char)) * (strlen(pwd) + strlen(fname) + 2));
    if(filepath_tmp == NULL) {
        return;
    }
    
    strcpy(&filepath_tmp[0], pwd);
    filepath_tmp[strlen(pwd)] = '/';
    strcpy(&filepath_tmp[strlen(pwd)+1], fname);

    *filepath = filepath_tmp;

    return;
}

// Directory walk routine: scans inodes in `pwd` in any order, collecting 
// statistics based on the type of inode encountered. See `stat_facts` for info
// on what is collected. Recursively descends into directory-type inodes. 
//
// Returns EXIT_OK if no critical errors were encountered.
//  A critical error is anything that *does not* destroy the integrity of
//  stat_facts
// Returns EXIT_FAIL if we cannot continue, or any subroutines fail.
int directory_walker(const char *pwd, stat_facts *fstree_stats)
{
    // collect facts on pwd - dir structure. make sure we are in a directory
    struct stat st;
    stat(pwd, &st);
    if((st.st_mode & S_IFMT) != S_IFDIR) {
        ERR_CLOSE("%s is not a directory!", pwd);
    }
    
    DIR *dirp;
    struct dirent *pwd_ent = NULL;
    if((dirp = opendir(pwd)) == NULL) {
        ERR_CLOSE("Could not open %s! %s", pwd); 
    }

    while((pwd_ent = readdir(dirp)) != NULL) {
        char *filename = pwd_ent->d_name;
        if(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
            continue;
        }

        // allocate full filepath from our root
        char *filepath = NULL;
        get_filepath(pwd, filename, &filepath);
        if(filepath == NULL) {
            ERR_CLOSE("Could not allocate name buffer for %s! Reason: %s",
                      filename);
        }

        // check for invalid characters -- anything not easily written by kbd
        for(int i = 0; i < strlen(filepath); i++) {
            if(!isascii(filepath[i]) || 
               isblank(filepath[i])  || 
               iscntrl(filepath[i])  ||
               filepath[i] == '\'' || filepath[i] == '\\' || filepath == "\"")
            {
                fstree_stats->bad_filename_count+=1;
                break;
            }
        }

        // observe inode, get mode
        if(lstat(filepath, &st) < 0) {
            ERR_CONT("Could not stat %s! Reason: %s", filepath);
        }

        int current_mode = (st.st_mode & S_IFMT);
        
        // check if its a hard link, and whether we have seen it or not.
        //  if we have seen this inode, we continue.
        if((st.st_nlink > 1)) {
            switch(hlink_processor(st.st_ino, fstree_stats)) {
                case EXIT_FAIL:
                    ERR_CLOSE("Failed to process inode hard-links! %s");
                    break; //not reached
                case EXIT_SEEN:
                    continue;
                default:
                    break;
            }
        }
        
        fstree_stats->inode_count[st_ind(current_mode)]+=1;
        // if this is a directory, continue down
        if(current_mode == S_IFDIR) {
            directory_walker(filepath, fstree_stats);
            continue;
        }

        switch (current_mode) {
        case S_IFREG:
            fstree_stats->file_size_sum+=st.st_size;
            fstree_stats->blk_alloc_sum+=st.st_blocks;
            break;
        case S_IFLNK:
            // open the other end to see if its dangling or not
            int fd;
            if((fd = open(filepath, O_RDONLY)) < 0) {
                if(errno == ENOENT) {
                    ERR_CONT("Could not stat %s! Reason: %s", filepath);
                    fstree_stats->dangling_symlink_count+=1;
                } else {
                    // It presumably exists, but there are other problems
                    ERR_CONT("Could not stat %s! Reason: %s", filepath);
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

// When given an index for `inode_count`, this is used to translate index ->
//  inode type. If theres no name associated, we return a null pointer
char *typefinder(int type) {
    switch (type << 12) { // we have to match the mask
        case S_IFLNK:
            return "symbolic link";
        case S_IFREG:
            return "regular file";
        case S_IFDIR:
            return "directory";
        case S_IFIFO:
            return "fifo";
        case S_IFCHR:
            return "character device";
        case S_IFBLK:
            return "block device";
        case S_IFSOCK:
            return "socket";
        default:
            return NULL;
    }
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
 
    // Print stats
    printf("\n");
    for(long unsigned int i = 0; i < 16; i++) {
        long unsigned int count = fstree_stats.inode_count[i];
        if(count == 0) { continue; }

        char *type = typefinder(i);
        if(type == NULL) {
            printf("There are %lu inodes of type %lu\n",
                   count, i);
        } else {
            printf("There are %lu \"%s\" inodes\n",
                   count, type);
        }
    }

    printf("Total size in bytes of all encountered regular file inodes: %lu\n", 
           fstree_stats.file_size_sum);
    printf("Total number of allocated disk blocks: %lu\n",
           fstree_stats.blk_alloc_sum);
    printf("Total number of (non-directory) inodes w/ more than 1 hard link: %lu\n",
           fstree_stats.hlinked_inode_count);
    printf("Total number of dangling symlinks: %lu\n",
           fstree_stats.dangling_symlink_count);
    printf("Total number of pathnames with problematic names: %lu\n",
           fstree_stats.bad_filename_count);
    return EXIT_OK;
}
