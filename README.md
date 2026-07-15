use following to build for arm64:
```make ARCH=arm64```

to remove built modules (do it before pushing.):
```make clean```

in makefile under arm64 , update your kernel source location for target
 
source on 'test board' is <https://github.com/nxp-imx/linux-imx.git>
branch is <lf-6.1.y> and commit hash is : <66e442bc7fdc>

to use the same source as 'test board' do :
 
```
git clone --single-branch -b <branch> <link>
cd linux-imx
git checkout <commit hash>
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
make imx_v8_defconfig
make modules_prepare
```
