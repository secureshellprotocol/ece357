#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define EXIT_OK      0
#define EXIT_FAIL   -1

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
    int hlink_count;            // Counts no. of encountered hard links
    int dangling_symlink_count; // Counts no. of unresolvable symlinks
    int bad_filename_count;     // Counts no. of file names w/ bad names
                                //  ie: name contains control characters,
                                //  non-printable characters, or non-ASCII chars
    int inode_list
} stat_facts;

int directory_walker(const char *pwd, stat_facts *collected_stats)
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
            strcmp(pwd_ent->d_name, "..") == 0) {
            continue;
        }

        // stat our file
        // can be more elegant w/ fstatat, probably. dont care lol!
        char *filepath = 
            malloc((sizeof(char)) * (strlen(pwd) + strlen(pwd_ent->d_name) + 2));
        strcpy(filepath, pwd);
        strcpy(&filepath[strlen(pwd)], "/");
        strcpy(&filepath[strlen(pwd)+1], pwd_ent->d_name);


        if(lstat(filepath, &st) < 0){
            ERR_CONT("Could not stat %s! Reason: %s", filepath);
        }
        int current_mode = (st.st_mode & S_IFMT);
        collected_stats->inode_count[st_ind(current_mode)]+=1;
        
        // check if inode no. has been seen before


        switch (current_mode) {
        case S_IFREG:
            printf("%s", filepath);
            printf("  %d\n", st_ind(current_mode));
            collected_stats->file_size_sum+=st.st_size;
            collected_stats->blk_alloc_sum+=st.st_blocks;
            break;
        case S_IFDIR:
            directory_walker(filepath, collected_stats);
            break;
        case S_IFLNK:
            printf("%s", filepath);

            // check resolution
            if(open(filepath, O_RDONLY) < 0) {
                if(errno == ENOENT){
                    collected_stats->dangling_symlink_count+=1;
                    printf(" broken ");
                }
            }

            printf("  %d\n", st_ind(current_mode));
            break;
        default:
            // dont care
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

    stat_facts collected_stats = {
        { 0 },  // inode_count[]
        0,      // file_size_sum
        0,      // blk_alloc_sum
        0,      // hlink_count
        0,      // dangling_symlink_count
        0       // bad_filename_count
    };
    int pp = st_ind(S_IFDIR);
    collected_stats.inode_count[st_ind(S_IFDIR)]+=1; // Our root inode!
    if(directory_walker(argv[1], &collected_stats) < 0){
        ERR_CLOSE("Directory walk routine failed! on dir %s",argv[1]);
    }
    
    return EXIT_OK;
}


