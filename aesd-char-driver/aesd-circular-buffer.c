/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include "aesd-circular-buffer.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    uint8_t index;
    struct aesd_buffer_entry *entry;
    uint8_t total_bytes = 0;
    AESD_CIRCULAR_BUFFER_FOREACH(entry,buffer,index) {
        total_bytes += entry->size;
    }
    index = buffer->out_offs;
    entry = &(buffer->entry[index]);

    if( char_offset >= total_bytes ) {
        return NULL;
    }
    if (char_offset == 0) {
        *entry_offset_byte_rtn = 0;
        entry = &(buffer->entry[index]);
        return entry;
    }
    while( char_offset >= entry->size ) {
        char_offset -= entry->size;
        index = (index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entry = &(buffer->entry[index]);
    }
    if (char_offset == entry->size) {
        *entry_offset_byte_rtn = 0;
    }
    else {
        *entry_offset_byte_rtn = char_offset;
    }
    return entry;

}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    if(buffer == NULL || add_entry == NULL)
    {
        return NULL;
    }
    char* ptr = NULL;
    if (buffer->full) {
        ptr = (char*)buffer->entry[buffer->out_offs].buffptr;
        buffer->entry[buffer->in_offs] = *add_entry;
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        if(buffer->out_offs + 1 == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
            buffer->out_offs = 0;
        }
        else {
	    buffer->out_offs++;
	    }
    }
    else {
        buffer->entry[buffer->in_offs] = *add_entry;
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        if (buffer->in_offs == buffer->out_offs) {
            buffer->full = true;
            buffer->in_offs = 0;
        }

    }
    return ptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
