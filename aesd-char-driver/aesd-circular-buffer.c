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
    //current location of out_fs for local calculations
    size_t current_index  = buffer->out_offs;

    //step1: traverse through the buffer till the offset is bigger than current entry size to see bytes
    while(char_offset >= buffer->entry[current_index].size)
    {
        //step2:subtract the bytes to have a new offset 
        char_offset -= buffer->entry[current_index].size;

        //step3: advance the out_offs 
        current_index  = (current_index  + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        
        //step4:if offset exceeds last byte, return NULL
        if(current_index  == buffer->in_offs)
        {
            if(char_offset <= 0)
            {
                return NULL;
            }
        }

        //NULL if this position is not available in the buffer (not enough data is written).
        if(buffer->entry[current_index].buffptr == NULL)
        {
            return NULL;
        }
    }

    //step5: store bytes
    *entry_offset_byte_rtn = char_offset;

    //step6: return the struct aesd_buffer_entry structure representing the position described by char_offset
    return &(buffer->entry[current_index]);
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/


void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    // Step 1: Store buffer entry
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    buffer->entry[buffer->in_offs].size = add_entry->size;

    // Step 2: Update/increment the in_offs and handle buffer full condition
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = true;
        // If buffer is full, move out_offs to make space for the new entry
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Step 3: If buffer is full, move out_offs to prevent exceeding array bounds
    if (buffer->full) {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
