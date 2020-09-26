!#/bin/bash

curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../webapp/publicractive.js" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../webapp/public.css" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../webapp/webApp.html" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../webapp/publicapp.js" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../publicslaveinfo.htm" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../sequences/tease" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../sequences/specialtease" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../sequences/slowtease" {$1}/api/files
curl -v -F "user=user" -F "pwd=pwd" -F "filesize=40428" -F "file=@./../sequences/quicktease" {$1}/api/files
echo "done"


