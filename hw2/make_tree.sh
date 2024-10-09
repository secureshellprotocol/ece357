rm -r root
mkdir root
mkdir root/folder1
mkdir root/folder2
mkdir root/staten_island
mkdir root/staten_island/maintenance_facility

dd if=/dev/urandom of=root/staten_island/ferry bs=2K count=1
touch root/file1
touch root/file2
touch root/folder1/file3
mkdir root/Google\ Chrome.app
touch root/Google\ Chrome.app/\t.ok

ln -s ../file1 root/folder1/slink1
ln -s ../file99 root/folder2/slink2
ln root/file1 root/folder2/hlink1
ln root/staten_island/ferry root/staten_island/maintenance_facility/alice_austen
ln root/file1 root/folder2/hlink2
