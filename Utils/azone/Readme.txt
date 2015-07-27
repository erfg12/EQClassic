azone read me


Dark-Prince - 10/05/2008 : 

Currently using arena.s3d as a test file, it crashes on line:27 on s3d.c

This coudl be because s3d_fn_header->fncount is reporting as 10, however it only works with 8, and as such the rest of the process fails.

It could be due to the s3d files being older than azone,  so we will have to check it.



