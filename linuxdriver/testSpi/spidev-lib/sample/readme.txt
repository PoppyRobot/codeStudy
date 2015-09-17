1、编译步骤：

在spidev-lib目录下依次执行如下命令，成功后会生成libspidev-lib.a和spi-testc文件：

mkdir build

cd build

cmake ..

make 



2、运行测试程序：


把libspidev-lib.a和spi-testc文件拷贝到EMB3500开发板上，
在测试前要保证安装了中断处理的驱动“insmod fpga_irq_drv.ko”,
执行测试，中断需要和FPGA的一起才能测试。
