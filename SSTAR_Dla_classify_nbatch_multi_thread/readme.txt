����˵����
    a.�޸�makefile�еı�����Ϊϵͳ��Ӧ���������޸�PROJ_ROOT·��	
    b. make clean; make

1. demo�Լ�API��˵����ssu938x��API���÷����Լ�ע�����

2. demo flow˵��

dla_classifyNBatch_multiThread���ڶ��̶߳���ͬģ��channel���ݷ���ģ�͡�

*��demo��IPUCreateDeviceʱ����device������Ҫ����ipu_firmware·�����ڴ棩��
*��demo��model·����ȡmodel����ʾ���û����ͨ��MI_SYS_MMA_Alloc���ⲿ����model MMA�ڴ棨�Զ�4KB���룩����ͨ��MI_SYS_Mmap��model MMA PA(��Ҫ4KB����)ӳ���cached VA��
*��demo��IPUCreateChannel_FromUserMMAMemoryʱ�����ⲿ�����model MMA�ڴ��ַ����(��Ҫ4KB����)������channelͨ��������
*��demo��MI_IPU_GetOfflineModeStaticInfo�л�ȡ����ģ��������Ҫ��variable buffer size������ģ���ļ���С��
*��demo��MI_IPU_GetInOutTensorDesc�л�ȡ��channelͨ���󶨵�����ģ�͵����������Ϣ��
*��demo��ʾ��������û��Լ�ͨ��MI_SYS_MMA_Alloc����in/out tensor��MMA�ڴ��Լ�variable buffer��MMA�ڴ棬��ͨ��MI_SYS_Mmap��MMA PA(��Ҫ4KB����)ӳ���cached VA��
*��demo��ʾ�����ʹ��MI_SYS_FlushInvCacheȥflush cached VA�ڴ档
*��demo��MI_IPU_Invoke2�н���ģ��������Ҫ�����û��Լ������in/out tensors MMA�ڴ�(ÿ��tensorBuffer��Ҫ4KB����)��variable buffer MMA�ڴ�(��Ҫ4KB����)�ʹ�С
*��demo��ʾ����MI_IPU_Invoke2������Ҫ��in/out tensor�ڴ��layout��ʽ��
*��demo invoke��batch��Ϊ3������batchʹ��ͬһ��inputͼƬ��������ֻ��ʾbatch 0�������

*��demo��ʾ����������̴߳�����һ��channel�����������߳���ʹ����ͬ��channelͬʱinvoke��ͬ������picture��
*�����̷ֱ߳����û��Լ�alloc���Լ���in/out tensor buffer(MI_SYS_MMA_Alloc�ĵ�ַ�Զ�4KB����)�Լ�variable buffer(MI_SYS_MMA_Alloc�ĵ�ַ�Զ�4KB����)�������̵߳�invoke�ֱ����ڲ�ͬ��IPU core���档

3. demo���з�ʽ
���пɻ��ʹ�÷�����ʾ
./prog_dla_classifyNBatch_multiThread
USAGE: ./prog_dla_classifyNBatch_multiThread: <xxxsgsimg.img> <picture1> <picture2> <labels> <model intput_format:RGB or BGR>


���У�
xxxsgsimg.img -> Ϊ����imgģ��·��
picture1 -> ΪͼƬ1·��
picture2 -> ΪͼƬ2·��
labels -> Ϊ��ǩ·��
model intput_format:RGB or BGR -> ѡ��RGB��BGR

ע�⣺
��Demo��������input_config.ini��input_formats����ΪRGB��BGR�ķ���ģ�͡�
��Demoʾ���˴��ڴ��ȡģ�͵ķ�����
