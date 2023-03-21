### 初始化仓库
~~~shell
git init
~~~


### 添加文件到版本库（只是添加到缓存区），.代表添加文件夹下所有文件
~~~shell
git add .
git add ./a.txt
~~~


### 删除缓存
~~~shell
git rm -r -f --cached ./ 
~~~

### 把添加的文件提交到版本库，并填写提交备注(必不可少)
~~~shell
git commit -m “first commit”
~~~

### 把本地库与远程库关联（如果已经有origin关联则可以忽略）
~~~shell
git remote add origin 你的远程库地址
~~~

### 推送（提交）
* git push origin(主机名) master(本地分支名):master(远程分支名)
~~~shell
git push <远程主机名> <本地分支名>:<远程分支名>
git push -u origin master #将仓库的代码 push到master分支上
~~~

### 直接修改最新一次的commit
~~~shell
git commit --amend
~~~


### 强制推送
~~~shell
git push -f origin master
~~~


### 查看仓库状态
* git status


### 拉取合并
* git pull
~~~shell
git pull --rebase origin master
~~~


### 删除关联的origin的远程库
~~~shell
git remote rm origin 远程库
~~~