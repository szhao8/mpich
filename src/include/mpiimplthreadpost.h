/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* this file contains definitions and declarations that would otherwise live in
 * mpiimplthread.h but must come later in the mpiimpl.h include process because
 * of the "big ball of wax" situation in our headers */

#ifndef MPIIMPLTHREADPOST_H_INCLUDED
#define MPIIMPLTHREADPOST_H_INCLUDED

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
/* All the funcs below require MPID Thread defns - that are available only
 * when thread level >= serialized
 */
#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_enter_lockname_impl_
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* these are inline functions instead of macros to avoid some of the
 * MPIU_THREADPRIV_DECL scoping issues */
MPIU_DBG_ATTRIBUTE_NOINLINE
static MPIU_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_enter_lockname_impl_(enum MPIU_Nest_mutexes kind,
                                    const char *lockname,
                                    MPID_Thread_mutex_t *mutex)
{
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    MPIU_THREAD_CHECKDEPTH(kind, lockname, 0);
    MPIU_THREAD_CHECKNEST(kind, lockname)
    {
        MPIU_DBG_MSG_S(THREAD,TYPICAL,"locking %s", lockname);
        MPID_Thread_mutex_lock(mutex);
        MPIU_THREAD_UPDATEDEPTH(kind, lockname, 1);
    }
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_exit_lockname_impl_
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
MPIU_DBG_ATTRIBUTE_NOINLINE
static MPIU_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_exit_lockname_impl_(enum MPIU_Nest_mutexes kind,
                                   const char *lockname,
                                   MPID_Thread_mutex_t *mutex)
{
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    MPIU_THREAD_CHECKDEPTH(kind, lockname, 1);
    MPIU_THREAD_CHECKNEST(kind, lockname)
    {
        MPIU_DBG_MSG_S(THREAD,TYPICAL,"unlocking %s", lockname);
        MPID_Thread_mutex_unlock(mutex);
        MPIU_THREAD_UPDATEDEPTH(kind, lockname, -1);
    }
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_yield_lockname_impl_
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
MPIU_DBG_ATTRIBUTE_NOINLINE
static MPIU_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_yield_lockname_impl_(enum MPIU_Nest_mutexes kind,
                                    const char *lockname,
                                    MPID_Thread_mutex_t *mutex)
{
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    MPIU_THREAD_CHECKDEPTH(kind, lockname, 1);
    /* don't CHECKNEST here, we want nesting to be >0 */
    MPID_Thread_mutex_unlock(mutex);
    /* FIXME should we MPID_Thread_yield() here? */
    MPID_Thread_mutex_lock(mutex);
}

/* ------------------------------------------------------------------- */
/* recursive versions, these are for the few locks where it is difficult or
 * impossible to eliminate recursive locking usage */

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_enter_lockname_recursive_impl_
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* these are inline functions instead of macros to avoid some of the
 * MPIU_THREADPRIV_DECL scoping issues */
MPIU_DBG_ATTRIBUTE_NOINLINE
static MPIU_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_enter_lockname_recursive_impl_(enum MPIU_Nest_mutexes kind,
                                              const char *lockname,
                                              MPID_Thread_mutex_t *mutex)
{
    int depth;
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    depth = MPIU_THREADPRIV_FIELD(lock_depth)[kind];
    MPIU_Assert(depth >= 0 && depth < 10); /* probably a mismatch if we hit this */
    MPIU_DBG_MSG_D(THREAD,TYPICAL,"recursive enter, depth=%d", depth);

    if (depth == 0) {
        MPIU_DBG_MSG_S(THREAD,TYPICAL,"locking %s", lockname);
        MPID_Thread_mutex_lock(mutex);
    }
    MPIU_THREADPRIV_FIELD(lock_depth)[kind] += 1;
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_exit_lockname_recursive_impl_
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
MPIU_DBG_ATTRIBUTE_NOINLINE
static MPIU_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_exit_lockname_recursive_impl_(enum MPIU_Nest_mutexes kind,
                                             const char *lockname,
                                             MPID_Thread_mutex_t *mutex)
{
    int depth;
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    depth = MPIU_THREADPRIV_FIELD(lock_depth)[kind];
    MPIU_Assert(depth > 0 && depth < 10); /* probably a mismatch if we hit this */
    MPIU_DBG_MSG_D(THREAD,TYPICAL,"recursive exit, depth=%d", depth);

    if (depth == 1) {
        MPIU_DBG_MSG_S(THREAD,TYPICAL,"unlocking %s", lockname);
        MPID_Thread_mutex_unlock(mutex);
    }
    MPIU_THREADPRIV_FIELD(lock_depth)[kind] -= 1;
}

#undef FUNCNAME
#define FUNCNAME MPIU_Thread_CS_yield_lockname_recursive_impl_
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
MPIU_DBG_ATTRIBUTE_NOINLINE
static MPIU_DBG_INLINE_KEYWORD void
MPIU_Thread_CS_yield_lockname_recursive_impl_(enum MPIU_Nest_mutexes kind,
                                              const char *lockname,
                                              MPID_Thread_mutex_t *mutex)
{
    int depth;
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;
    depth = MPIU_THREADPRIV_FIELD(lock_depth)[kind];
    MPIU_Assert(depth > 0 && depth < 10); /* we must hold the mutex */
    /* no need to update depth, this is a thread-local value */

    MPIU_DBG_MSG_D(THREAD,TYPICAL,"recursive yield, depth=%d", depth);

    MPID_Thread_mutex_unlock(mutex);
    /* FIXME should we MPID_Thread_yield() here? */
    MPID_Thread_mutex_lock(mutex);
}

/* undef for safety, this is a commonly-included header */
#undef FUNCNAME
#undef FCNAME

#endif /*  (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED) */

#endif /* defined(MPIIMPLTHREADPOST_H_INCLUDED) */

