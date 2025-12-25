GET/files

http://120.79.89.230:8001/files

摘要: 获取文件列表
描述: 获取服务器上所有文件的列表

{"files":[{"filename":"wallpaper.bmp","size":16262,"url":"/files/wallpaper.bmp","original_filename":"wallpaper.jpg","filetype":"image/bmp","upload_time":"2025-12-25T20:29:37.527957","thumbnail":"/files/thumbs/wallpaper.bmp"}]}

----

GET/files/{filename}

example: http://120.79.89.230:8001/files/wallpaper.bmp

摘要: 获取单个文件
描述: 根据文件名获取文件

----

GET/files/thumbs/{filename}


http://120.79.89.230:8001/files/thumbs/wallpaper.bmp

摘要: 获取缩略图
描述: 根据文件名获取缩略图