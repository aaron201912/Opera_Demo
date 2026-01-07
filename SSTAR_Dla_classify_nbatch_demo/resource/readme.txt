编译说明：
    a.修改makefile中的编译链为系统对应编译链

    b.如果将demo放到跟project同级目录   （默认方式）
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

    c.demo放置到任意路径，只需在make的时候指定PROJ_ROOT到SDK工程project的目录（需要链接SDK头文件）
    --> declare -x PROJ_ROOT=~/sdk/source_code/${myprojectpath}
    --> make clean; make TOOLCHAIN_VERSION=6.4.0 OR make clean; make TOOLCHAIN_VERSION=10.2.1

1. demo以及API均说明了ssu938x的API调用方法以及注意事项。

2. demo flow说明

dla_classifyNBatch用于推演分类模型。

*该demo在IPUCreateDevice时创建device（不需要传入ipu_firmware路径或内存）。
*该demo从model路径读取model，演示了用户如何通过MI_SYS_MMA_Alloc在外部分配model MMA内存（自动4KB对齐），并通过MI_SYS_Mmap将model MMA PA(需要4KB对齐)映射成cached VA。
*该demo在IPUCreateChannel_FromUserMMAMemory时，将外部分配的model MMA内存地址传入(需要64bytes对齐)，用于channel通道创建。
*该demo在MI_IPU_GetOfflineModeStaticInfo中获取离线模型运行需要的variable buffer size和离线模型文件大小。
*该demo在MI_IPU_GetInOutTensorDesc中获取与channel通道绑定的离线模型的输入输出信息。
*该demo演示了如何由用户自己通过MI_SYS_MMA_Alloc分配in/out tensor的MMA内存以及variable buffer的MMA内存，并通过MI_SYS_Mmap将MMA PA(需要4KB对齐)映射成cached VA。
*该demo演示了如何使用MI_SYS_FlushInvCache去flush cached VA内存。
*该demo在MI_IPU_Invoke2中进行模型推理，需要传入用户自己分配的in/out tensors MMA内存(每个tensorBuffer需要64bytes对齐)、variable buffer MMA内存(需要64bytes对齐)和大小
*该demo演示了在MI_IPU_Invoke2中所需要的in/out tensor内存的layout格式。
*该demo invoke的batch数为手动输入，每个batch使用同一个input图片。输出结果所有batch的分类输出。
*该demo演示了如何运行batch_one_buf模式的离线模型。
*该demo演示了运行batch_one_buf模式的离线模型时MI_IPU_Invoke2Custom的输入输出tensor的内存排布以及内存分配方法。
*该demo演示了如何从模型中获得该模型的batch mode是batch_one_buf模式还是batch_n_buf模式。

3. MI_IPU_API_V3.DOC中详细说明的API接口的作用以及使用注意事项。

4. mi_ipu.h、mi_ipu_datatype.h为IPU相关API的header，不包括MI_SYS系统相关API。

5. demo运行方式
运行可获得使用方法提示, 将resource和prog_dla_classifyNBatch拷贝到customer下
./prog_dla_classifyNBatch
USAGE: ./prog_dla_dla_classifyNBatch: <xxxsgsimg.img> <picture> <labels> <model intput_format:RGB or BGR> <batch number: 1~10>
cd /customer
./prog_dla_classifyNBatch resource/mobilenet_v2_fixed.sim_sgsimg.img resource/ILSVRC2012_test_00000002.bmp resource/labels.txt RGB 1


其中，
xxxsgsimg.img -> 为离线img模型路径
picture -> 为图片路径
labels -> 为标签路径
model intput_format:RGB or BGR -> 选择RGB或BGR
batch number: 1~10 -> 想要invoke的batch数。demo会根据该值去配置batch参数以及in/out buffer大小。


注意：
该Demo仅能运行input_config.ini中input_formats设置为RGB或BGR的分类模型。
该Demo示例了从内存读取模型的方法。
