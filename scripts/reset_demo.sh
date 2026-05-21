#!/bin/bash
find www/demo/upload/received -type f ! -name ".gitkeep" -delete
find www/default/upload/received -type f ! -name ".gitkeep" -delete
find www/demo/delete/files -type f ! -name ".gitkeep" -delete
find www/default/delete/files -type f ! -name ".gitkeep" -delete
echo "hello from webserv demo" > www/demo/delete/files/file1.txt
echo "delete me" > www/demo/delete/files/file2.txt
echo "another test file" > www/demo/delete/files/file3.txt
echo "hello from webserv default" > www/default/delete/files/file1.txt
echo "delete me" > www/default/delete/files/file2.txt
echo "demo files reset"
