����˵����
    a.�޸�makefile�еı�����Ϊϵͳ��Ӧ���������޸�PROJ_ROOT·��	
    b. make clean; make

1. demo�Լ�API��˵����ssu938x��API���÷����Լ�ע�����

2. demo flow˵��

dla_classifyNBatch�������ݷ���ģ�͡�

*��demo��IPUCreateDeviceʱ����device������Ҫ����ipu_firmware·�����ڴ棩��
*��demo��model·����ȡmodel����ʾ���û����ͨ��MI_SYS_MMA_Alloc���ⲿ����model MMA�ڴ棨�Զ�4KB���룩����ͨ��MI_SYS_Mmap��model MMA PA(��Ҫ4KB����)ӳ���cached VA��
*��demo��IPUCreateChannel_FromUserMMAMemoryʱ�����ⲿ�����model MMA�ڴ��ַ����(��Ҫ64bytes����)������channelͨ��������
*��demo��MI_IPU_GetOfflineModeStaticInfo�л�ȡ����ģ��������Ҫ��variable buffer size������ģ���ļ���С��
*��demo��MI_IPU_GetInOutTensorDesc�л�ȡ��channelͨ���󶨵�����ģ�͵����������Ϣ��
*��demo��ʾ��������û��Լ�ͨ��MI_SYS_MMA_Alloc����in/out tensor��MMA�ڴ��Լ�variable buffer��MMA�ڴ棬��ͨ��MI_SYS_Mmap��MMA PA(��Ҫ4KB����)ӳ���cached VA��
*��demo��ʾ�����ʹ��MI_SYS_FlushInvCacheȥflush cached VA�ڴ档
*��demo��MI_IPU_Invoke2�н���ģ��������Ҫ�����û��Լ������in/out tensors MMA�ڴ�(ÿ��tensorBuffer��Ҫ64bytes����)��variable buffer MMA�ڴ�(��Ҫ64bytes����)�ʹ�С
*��demo��ʾ����MI_IPU_Invoke2������Ҫ��in/out tensor�ڴ��layout��ʽ��
*��demo invoke��batch��Ϊ�ֶ����룬ÿ��batchʹ��ͬһ��inputͼƬ������������batch�ķ��������
*��demo��ʾ���������batch_one_bufģʽ������ģ�͡�
*��demo��ʾ������batch_one_bufģʽ������ģ��ʱMI_IPU_Invoke2Custom���������tensor���ڴ��Ų��Լ��ڴ���䷽����
*��demo��ʾ����δ�ģ���л�ø�ģ�͵�batch mode��batch_one_bufģʽ����batch_n_bufģʽ��

3. MI_IPU_API_V3.DOC����ϸ˵����API�ӿڵ������Լ�ʹ��ע�����

4. mi_ipu.h��mi_ipu_datatype.hΪIPU���API��header��������MI_SYSϵͳ���API��

5. demo���з�ʽ
���пɻ��ʹ�÷�����ʾ
./prog_dla_classifyNBatch
USAGE: ./prog_dla_dla_classifyNBatch: <xxxsgsimg.img> <picture> <labels> <model intput_format:RGB or BGR> <batch number: 1~10>


���У�
xxxsgsimg.img -> Ϊ����imgģ��·��
picture -> ΪͼƬ·��
labels -> Ϊ��ǩ·��
model intput_format:RGB or BGR -> ѡ��RGB��BGR
batch number: 1~10 -> ��Ҫinvoke��batch����demo����ݸ�ֵȥ����batch�����Լ�in/out buffer��С��


ע�⣺
��Demo��������input_config.ini��input_formats����ΪRGB��BGR�ķ���ģ�͡�
��Demoʾ���˴��ڴ��ȡģ�͵ķ�����
