编译说明：
    a.修改makefile中的编译链为系统对应编译链，修改PROJ_ROOT路径	
    b. make clean; make

1. demo以及API均说明了ssu938x的API调用方法以及注意事项。

2. demo flow说明

dla_classifyNBatch_multiThread用于多线程对相同模型channel推演分类模型。

*该demo在IPUCreateDevice时创建device（不需要传入ipu_firmware路径或内存）。
*该demo从model路径读取model，演示了用户如何通过MI_SYS_MMA_Alloc在外部分配model MMA内存（自动4KB对齐），并通过MI_SYS_Mmap将model MMA PA(需要4KB对齐)映射成cached VA。
*该demo在IPUCreateChannel_FromUserMMAMemory时，将外部分配的model MMA内存地址传入(需要4KB对齐)，用于channel通道创建。
*该demo在MI_IPU_GetOfflineModeStaticInfo中获取离线模型运行需要的variable buffer size和离线模型文件大小。
*该demo在MI_IPU_GetInOutTensorDesc中获取与channel通道绑定的离线模型的输入输出信息。
*该demo演示了如何由用户自己通过MI_SYS_MMA_Alloc分配in/out tensor的MMA内存以及variable buffer的MMA内存，并通过MI_SYS_Mmap将MMA PA(需要4KB对齐)映射成cached VA。
*该demo演示了如何使用MI_SYS_FlushInvCache去flush cached VA内存。
*该demo在MI_IPU_Invoke2中进行模型推理，需要传入用户自己分配的in/out tensors MMA内存(每个tensorBuffer需要4KB对齐)、variable buffer MMA内存(需要4KB对齐)和大小
*该demo演示了在MI_IPU_Invoke2中所需要的in/out tensor内存的layout格式。
*该demo invoke的batch数为3，三个batch使用同一个input图片。输出结果只显示batch 0的输出。

*该demo演示了如何在主线程创建完一个channel后，在两个子线程中使用相同的channel同时invoke不同的输入picture。
*两个线程分别由用户自己alloc了自己的in/out tensor buffer(MI_SYS_MMA_Alloc的地址自动4KB对齐)以及variable buffer(MI_SYS_MMA_Alloc的地址自动4KB对齐)，两个线程的invoke分别跑在不同的IPU core上面。

3. demo运行方式
运行可获得使用方法提示
./prog_dla_classifyNBatch_multiThread
USAGE: ./prog_dla_classifyNBatch_multiThread: <xxxsgsimg.img> <picture1> <picture2> <labels> <model intput_format:RGB or BGR>


其中，
xxxsgsimg.img -> 为离线img模型路径
picture1 -> 为图片1路径
picture2 -> 为图片2路径
labels -> 为标签路径
model intput_format:RGB or BGR -> 选择RGB或BGR

注意：
该Demo仅能运行input_config.ini中input_formats设置为RGB或BGR的分类模型。
该Demo示例了从内存读取模型的方法。
