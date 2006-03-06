/*
 *     AAA    M M    OOO    SSSS
 *    A   A  M M M  O   O  S 
 *    AAAAA  M M M  O   O   SSS
 *    A   A  M   M  O   O      S
 *    A   A  M   M   OOO   SSSS 
 *
 *    Author:  Stephen Fewer
 *    Contact: steve [AT] harmonysecurity [DOT] com
 *    Web:     http://amos.harmonysecurity.com/
 *    License: GNU General Public License (GPL)
 */

#include <kernel/syscall.h>
#include <kernel/mm/mm.h>
#include <kernel/pm/scheduler.h>
#include <kernel/pm/sync/mutex.h>
#include <kernel/fs/vfs.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <lib/string.h>

struct SYSCALL syscall_table[SYSCALL_MAXCALLS];

int syscall_open( struct PROCESS_INFO * process, char * filename, int mode )
{
	int handleIndex;
	// find a free handle entry
	for( handleIndex=0; handleIndex<PROCESS_MAXHANDLES ; handleIndex++ )
	{
		if( process->handles[ handleIndex ] == NULL )
			break;
	}
	// see if we have gone over the limit
	if( handleIndex >= PROCESS_MAXHANDLES )
		return FAIL;
	// call the real function
	process->handles[ handleIndex ] = vfs_open( filename, mode );
	// check if the call failed
	if( process->handles[ handleIndex ] == NULL )
		return FAIL;
	// return the handle index
	return handleIndex;
}

int syscall_close( struct PROCESS_INFO * process, int handleIndex )
{
	// make sure the handle index is in range
	if( handleIndex < 0 || handleIndex >= PROCESS_MAXHANDLES )
		return FAIL;
	return vfs_close( process->handles[ handleIndex ] );	
}

int syscall_read( struct PROCESS_INFO * process, int handleIndex, BYTE * buffer, DWORD size )
{
	// make sure the handle index is in range
	if( handleIndex < 0 || handleIndex >= PROCESS_MAXHANDLES )
		return FAIL;
	return vfs_read( process->handles[ handleIndex ], buffer, size );
}

int syscall_write( struct PROCESS_INFO * process, int handleIndex, BYTE * buffer, DWORD size  )
{
	// make sure the handle index is in range
	if( handleIndex < 0 || handleIndex >= PROCESS_MAXHANDLES )
		return FAIL;
	return vfs_write( process->handles[ handleIndex ], buffer, size );
}

int syscall_seek( struct PROCESS_INFO * process, int handleIndex, DWORD offset, BYTE origin )
{
	// make sure the handle index is in range
	if( handleIndex < 0 || handleIndex >= PROCESS_MAXHANDLES )
		return FAIL;
	return vfs_seek( process->handles[ handleIndex ], offset, origin );	
}

int syscall_control( struct PROCESS_INFO * process, int handleIndex, DWORD request, DWORD arg )
{
	// make sure the handle index is in range
	if( handleIndex < 0 || handleIndex >= PROCESS_MAXHANDLES )
		return FAIL;
	return vfs_control( process->handles[ handleIndex ], request, arg );		
}

void * syscall_morecore( struct PROCESS_INFO * process, DWORD size )
{
	return mm_morecore( process, size );
}

int syscall_exit( struct PROCESS_INFO * process )
{
	if( process_kill( process->id ) == SUCCESS )
		return SYSCALL_SWITCH;
	return FAIL;
}

int syscall_wait( struct PROCESS_INFO * process, int id )
{
	return process_wait( id );
}

int syscall_spawn( struct PROCESS_INFO * process, char * filename, char * console_path )
{
	return process_spawn( process, filename, console_path );
}

int syscall_kill( struct PROCESS_INFO * process, int id )
{
	int ret = process_kill( id );
	if( ret == SUCCESS && process->id == id )
		return SYSCALL_SWITCH;
	return ret;
}

int syscall_sleep( struct PROCESS_INFO * process )
{
	return process_sleep( process );
}

int syscall_wake( struct PROCESS_INFO * process, int id )
{
	return process_wake( id );
}

int syscall_list( struct PROCESS_INFO * process, char * dir, struct VFS_DIRLIST_ENTRY * entry, int entry_num )
{
	struct VFS_DIRLIST_ENTRY * kentry;
	
	if( dir == NULL || entry == NULL || entry_num <= 0 )
		return FAIL;
		
	kentry = vfs_list( dir );
	if( kentry == NULL )
		return FAIL;
	
	memset( entry, 0x00, sizeof(struct VFS_DIRLIST_ENTRY)*entry_num );
	// bounds check this so it doesnt run over the kentry size
	memcpy( entry, kentry, sizeof(struct VFS_DIRLIST_ENTRY)*entry_num );
	
	mm_free( kentry );
	
	return SUCCESS;
}

struct PROCESS_INFO * syscall_handler( struct PROCESS_INFO * process )
{
	struct PROCESS_STACK kstack;
	int ret = FAIL;
	int index = (int)process->kstack->eax;
	// save the state of this process's kernel stack
	memcpy( &kstack, process->kstack, sizeof(struct PROCESS_STACK) );
	// make sure our syscall index into the syscall table is in range
	if( index < SYSCALL_MININDEX || index > SYSCALL_MAXINDEX )
		return process;
	// make sure the syscall function has been set
	if( syscall_table[ index ].function.function != NULL )
	{
		switch( syscall_table[ index ].parameters  )
		{
			case 0:
				ret = SYSTEM_CALL0( syscall_table[ index ].function, process );
				break;
			case 1:
				ret = SYSTEM_CALL1( syscall_table[ index ].function, process );
				break;
			case 2:
				ret = SYSTEM_CALL2( syscall_table[ index ].function, process );
				break;	
			case 3:
				ret = SYSTEM_CALL3( syscall_table[ index ].function, process );
				break;
			default:
				break;
		}
	}
	// if the system call failed and we need to perform a context switch
	if( ret == SYSCALL_SWITCH )
		return scheduler_select( process->prev );
	// restore the kernel stack for the jump back to user land
	memcpy( process->kstack, &kstack, sizeof(struct PROCESS_STACK) );
	// set return value
	process->kstack->eax = (DWORD)ret;
	// return to caller
	return process;
}

int syscall_add( int index, void * function, int parameters )
{
	// make sure our syscall index into the syscall table is in range
	if( index < SYSCALL_MININDEX || index > SYSCALL_MAXINDEX )
		return FAIL;
	// set the function & its number of parameters
	syscall_table[ index ].function.function = function;
	syscall_table[ index ].parameters = parameters;
	return SUCCESS;
}

int syscall_init( void )
{
	int index;
	// clear the system call table
	for( index=0 ; index<SYSCALL_MAXCALLS ; index++ )
	{	
		syscall_table[ index ].function.function = NULL;
		syscall_table[ index ].parameters = 0;
	}
	// add in all our system calls... file operations
	syscall_add( SYSCALL_OPEN,     syscall_open,	   2 );
	syscall_add( SYSCALL_CLOSE,    syscall_close,	   1 );
	syscall_add( SYSCALL_READ,     syscall_read,       3 );
	syscall_add( SYSCALL_WRITE,    syscall_write,      3 );
	syscall_add( SYSCALL_SEEK,     syscall_seek,       3 );
	syscall_add( SYSCALL_CONTROL,  syscall_control,    3 );	
	// file system operations
	/*syscall_add( SYSCALL_MOUNT,    vfs_mount,          3 );
	syscall_add( SYSCALL_UNMOUNT,  vfs_unmount,        1 );
	syscall_add( SYSCALL_CREATE,   vfs_create,         1 );
	syscall_add( SYSCALL_DELETE,   vfs_delete,         1 );
	syscall_add( SYSCALL_RENAME,   vfs_rename,         2 );
	syscall_add( SYSCALL_COPY,     vfs_copy,           2 );*/
	syscall_add( SYSCALL_LIST,     syscall_list,       3 );
	
	// memory operations
	syscall_add( SYSCALL_MORECORE, syscall_morecore,   1 );
	// process operations
	syscall_add( SYSCALL_SPAWN,    syscall_spawn,      2 );
	syscall_add( SYSCALL_KILL,     syscall_kill,       1 );
	syscall_add( SYSCALL_SLEEP,    syscall_sleep,      0 );
	syscall_add( SYSCALL_WAKE,     syscall_wake,       1 );
	syscall_add( SYSCALL_WAIT,     syscall_wait,       1 );
	syscall_add( SYSCALL_EXIT,     syscall_exit,       0 );
	
	// enable the system call interrupt, we set the privilage to
	// USER (DPL = RING3) so it may be accessed from user mode
	interrupt_enable( SYSCALL_INTERRUPT, syscall_handler, USER );
	return SUCCESS;
}
