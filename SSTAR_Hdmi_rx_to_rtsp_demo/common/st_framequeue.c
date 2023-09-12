#include <sys/time.h>

#include "st_framequeue.h"


static int _g_use_buf_mode = 0;
int frame_queue_init(frame_queue_t *f, int max_size, int frame_size)
{
    int i;
    memset(f, 0, sizeof(frame_queue_t));

    CheckFuncResult(pthread_mutex_init(&f->mutex, NULL));
    CheckFuncResult(pthread_cond_init(&f->cond,NULL));

    f->max_size = max_size;
    f->size = 0;
    for (i = 0; i < f->max_size; i++)
    {
        if(frame_size != 0)
        {
            if (!(f->queue[i].frame = (char *)malloc(frame_size)))
            {
                return -1;
            }
            _g_use_buf_mode = 1;
        }
        else
        {
            f->queue[i].frame = NULL;
        }
        f->queue[i].buf_size = frame_size;
        CheckFuncResult(pthread_mutex_init(&f->queue[i].mutex, NULL));
    }
    return 0;
}

void frame_queue_putbuf(frame_queue_t *f,char *frame,int buf_len ,void * handle)
{
    //struct timeval timeEnqueue;
    int windex;
    windex = f->windex;
    //printf("frame_queue_putbuf begin f=%llx rindex=%d \n",f,f->rindex);
    //pthread_mutex_lock(&f->mutex);
    pthread_mutex_lock(&f->queue[f->windex].mutex);
    //f->queue[f->windex].enqueueTime = (int64_t)timeEnqueue.tv_sec * 1000000 + timeEnqueue.tv_usec;
    if(handle != NULL)
    {
        f->queue[f->windex].handle = handle;
    }
    if(buf_len > 0 && (f->queue[f->windex].frame != NULL))
    {
        memcpy(f->queue[f->windex].frame,frame,buf_len);
        f->queue[f->windex].buf_size = buf_len;
    }
    else
    {
        f->queue[f->windex].frame = frame;
        f->queue[f->windex].buf_size = buf_len;
    }

    if (++f->windex == f->max_size)
    {
        f->windex = 0;
    }
    if(f->size < f->max_size)
        f->size++;
    pthread_mutex_unlock(&f->queue[windex].mutex);
    //pthread_mutex_unlock(&f->mutex);
    //pthread_cond_signal(&f->cond);
    //printf("frame_queue_putbuf end f=%llx rindex=%d \n",f,f->rindex);
}

#if 0

// 向队列尾部压入一帧，只更新计数与写指针，因此调用此函数前应将帧数据写入队列相应位置
void frame_queue_push(frame_queue_t *f)
{
    if (++f->windex == f->max_size)
    {
        f->windex = 0;
    }
    pthread_mutex_lock(&f->mutex);
    f->size++;
    pthread_mutex_unlock(&f->mutex);
    pthread_cond_signal(&f->cond);
}
#endif


//frame_queue_peek_lasty用完之后要call frame_queue_next归还

frame_t *frame_queue_peek_last(frame_queue_t *f,int wait_ms)
{

#if 0

    struct timespec now_time;
    struct timespec out_time;
    unsigned long now_time_us;
    clock_gettime(CLOCK_MONOTONIC, &now_time);
    out_time.tv_sec = now_time.tv_sec;
    out_time.tv_nsec = now_time.tv_nsec;
    out_time.tv_sec += wait_ms/1000;   //ms 可能超1s

    now_time_us = out_time.tv_nsec/1000 + 1000*(wait_ms%1000); //计算us
    out_time.tv_sec += now_time_us/1000000; //us可能超1s
    now_time_us = now_time_us%1000000;
    out_time.tv_nsec = now_time_us * 1000;//us
#endif
    //printf("frame_queue_peek_last begin f=%llx rindex=%d \n",f,f->rindex);
    //pthread_mutex_lock(&f->mutex);
    //pthread_cond_timedwait(&f->cond,&f->mutex,&out_time);
    //pthread_cond_wait(&f->cond,&f->mutex);
    pthread_mutex_lock(&f->queue[f->rindex].mutex);
    //printf("frame_queue_peek_last end f=%llx  rindex=%d \n",f,f->rindex);
    return &f->queue[f->rindex];
}


void frame_queue_next(frame_queue_t *f,frame_t* pFrame)
{
    //printf("frame_queue_next begin f=%llx rindex=%d \n",f,f->rindex);
    //pthread_mutex_unlock(&f->mutex);
    if(_g_use_buf_mode)
    {
        memset(f->queue[f->rindex].frame, 0, pFrame->buf_size);
    }
    else
    {
        f->queue[f->rindex].frame = NULL;
    }
    pFrame->buf_size = 0;
    f->queue[f->rindex].enqueueTime = 0;

    if (++f->rindex == f->max_size)
        f->rindex = 0;
    //pthread_mutex_lock(&f->mutex);
    if(f->size > 0)
	    f->size--;
    pthread_mutex_unlock(&pFrame->mutex);
    //pthread_mutex_unlock(&f->mutex);
    //printf("frame_queue_next end f=%llx rindex=%d \n",f,f->rindex);
}

void frame_queue_peek_end(frame_queue_t *f)
{
    pthread_mutex_unlock(&f->queue[f->rindex].mutex);
}

void frame_queue_flush(frame_queue_t *f)
{
    //printf("queue valid size : %d, rindex : %d\n", f->size, f->rindex);
    pthread_mutex_lock(&f->mutex);
    for (; f->size > 0; f->size --)
    {
        frame_t *vp = &f->queue[(f->rindex ++) % f->max_size];
        if(vp->frame)
        {
            if(_g_use_buf_mode && vp->frame != NULL)
            {
                free(vp->frame);
                vp->frame = NULL;
            }
        }
        if (f->rindex >= f->max_size)
            f->rindex = 0;
    }
    f->rindex = 0;
    f->windex = 0;
    f->size   = 0;
    pthread_mutex_unlock(&f->mutex);
}

void frame_queue_destory(frame_queue_t *f)
{
    int i;
    for (i = 0; i < f->max_size; i++) {
        frame_t *vp = &f->queue[i];
        if(vp->frame)
        {
            if(_g_use_buf_mode && vp->frame != NULL)
            {
                free(vp->frame);
                vp->frame = NULL;
            }
        }
    }
    pthread_mutex_destroy(&f->mutex);
    pthread_cond_destroy(&f->cond);
}







