
#include <assert.h>

#include <zebra-lock.h>

int zebra_mutex_init (Zebra_mutex *p)
{
    pthread_mutex_init (&p->mutex, 0);
    return 0;
}

int zebra_mutex_destroy (Zebra_mutex *p)
{
    pthread_mutex_destroy (&p->mutex);
    return 0;
}

int zebra_mutex_lock (Zebra_mutex *p)
{
    pthread_mutex_lock (&p->mutex);
    return 0;
}

int zebra_mutex_unlock (Zebra_mutex *p)
{
    pthread_mutex_unlock (&p->mutex);
    return 0;
}

int zebra_lock_rdwr_init (Zebra_lock_rdwr *p)
{
    p->readers_reading = 0;
    p->writers_writing = 0;
    pthread_mutex_init (&p->mutex, 0);
    pthread_cond_init (&p->lock_free, 0);
    return 0;
}

int zebra_lock_rdwr_destroy (Zebra_lock_rdwr *p)
{
    assert (p->readers_reading == 0);
    assert (p->writers_writing == 0);
    pthread_mutex_destroy (&p->mutex);
    pthread_cond_destroy (&p->lock_free);
    return 0;
}

int zebra_lock_rdwr_rlock (Zebra_lock_rdwr *p)
{
    pthread_mutex_lock (& p->mutex);
    while (p->writers_writing)
	pthread_cond_wait (&p->lock_free, &p->mutex);
    p->readers_reading++;
    pthread_mutex_unlock(&p->mutex);
    return 0;
}

int zebra_lock_rdwr_wlock (Zebra_lock_rdwr *p)
{
    pthread_mutex_lock (&p->mutex);
    while (p->writers_writing || p->readers_reading)
	pthread_cond_wait (&p->lock_free, &p->mutex);
    p->writers_writing++;
    pthread_mutex_unlock (&p->mutex);
    return 0;
}

int zebra_lock_rdwr_runlock (Zebra_lock_rdwr *p)
{
    pthread_mutex_lock (&p->mutex);
    if (p->readers_reading == 0)
    {
	pthread_mutex_unlock (&p->mutex);
	return -1;
    } 
    else
    {
	p->readers_reading--;
	if (p->readers_reading == 0)
	    pthread_cond_signal (&p->lock_free);
	pthread_mutex_unlock (&p->mutex);
    }
    return 0;
}

int zebra_lock_rdwr_wunlock (Zebra_lock_rdwr *p)
{
    pthread_mutex_lock (&p->mutex);
    if (p->writers_writing == 0)
    {
	pthread_mutex_unlock (&p->mutex);
	return -1;
    }
    else
    {
	p->writers_writing--;
	pthread_cond_broadcast(&p->lock_free);
	pthread_mutex_unlock(&p->mutex);
    }
    return 0;
}

int zebra_mutex_cond_init (Zebra_mutex_cond *p)
{
    pthread_cond_init (&p->cond, 0);
    pthread_mutex_init (&p->mutex, 0);
    return 0;
}

int zebra_mutex_cond_destroy (Zebra_mutex_cond *p)
{
    pthread_cond_destroy (&p->cond);
    pthread_mutex_destroy (&p->mutex);
    return 0;
}

int zebra_mutex_cond_lock (Zebra_mutex_cond *p)
{
    return pthread_mutex_lock (&p->mutex);
}

int zebra_mutex_cond_unlock (Zebra_mutex_cond *p)
{
    return pthread_mutex_unlock (&p->mutex);
}

int zebra_mutex_cond_wait (Zebra_mutex_cond *p)
{
    return pthread_cond_wait (&p->cond, &p->mutex);
}

int zebra_mutex_cond_signal (Zebra_mutex_cond *p)
{
    return pthread_cond_signal (&p->cond);
}

