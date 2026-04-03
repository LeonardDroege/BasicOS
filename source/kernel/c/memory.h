#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "multiboot.h"

#define PAGE_SIZE 4096

extern uint64_t mb_info_ptr;

typedef struct
{
  uint64_t base;
  uint64_t length;
} usable_region_t;

typedef struct kheap_block
{
	uint32_t size;
	uint8_t free;
	struct kheap_block* next;
} kheap_block_t;

usable_region_t usable_memory[64];
size_t usable_count = 0;

static uint32_t* pmm_bitmap;
static uint32_t  pmm_bitmap_size;   // in uint32_t entries
static uint32_t  pmm_page_count;    // total number of pages

extern uint32_t kernel_start;
extern uint32_t kernel_end;
extern uint32_t kernel_heap_start;
extern uint32_t kernel_heap_end;

static kheap_block_t* kheap_head = NULL;

static const char* to_hex32(uint32_t value) {
    static char buf[11]; // "0x" + 8 hex digits + null
    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 0; i < 8; i++) {
        uint8_t nibble = (value >> ((7 - i) * 4)) & 0xF;
        buf[2 + i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }

    buf[10] = 0;
    return buf;
}

static const char* to_hex64(uint64_t value) {
    static char buf[19]; // "0x" + 16 hex digits + null
    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 0; i < 16; i++) {
        uint8_t nibble = (value >> ((15 - i) * 4)) & 0xF;
        buf[2 + i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }

    buf[18] = 0;
    return buf;
}

static const char* to_dec(uint64_t value) {
    static char buf[32];
    int i = 30;
    buf[31] = 0;

    if (value == 0) {
        buf[30] = '0';
        return &buf[30];
    }

    while (value > 0 && i >= 0) {
        buf[i--] = '0' + (value % 10);
        value /= 10;
    }

    return &buf[i + 1];
}

void parse_memory_map(void)
{
	uint8_t* base = (uint8_t*)(uintptr_t)mb_info_ptr;

    uint32_t total_size = *(uint32_t*)base;
    uint8_t* ptr = base + 8;
    uint8_t* end = base + total_size;

    while(ptr < end)
    {
        struct multiboot_tag* tag = (void*)ptr;

        if(tag->type == MULTIBOOT_TAG_TYPE_END)
        {
            break;
        }

        if(tag->type == MULTIBOOT_TAG_TYPE_MMAP)
        {
            struct multiboot_tag_mmap* mmap_tag = (void*)tag;

            struct multiboot_mmap_entry* entry = mmap_tag->entries;
            struct multiboot_mmap_entry* entry_end = (struct multiboot_mmap_entry*)((uint8_t*)tag + tag->size);

            for(; entry < entry_end; entry = (struct multiboot_mmap_entry*)((uint8_t*)entry + mmap_tag->entry_size))
            {
                if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                {
                    uint64_t base_addr = entry->addr;
                    uint64_t len = entry->len;

                    if(base_addr < 0x100000000ULL)
                    {
                        uint64_t end_addr = base_addr + len;
                        if(end_addr > 0x100000000ULL)
                        {
                            end_addr = 0x100000000ULL;
                        }

                        usable_memory[usable_count].base = base_addr;
                        usable_memory[usable_count].length = end_addr - base_addr;
                        usable_count++;
                    }
                }
            }
        }

        ptr += (tag->size + MULTIBOOT_TAG_ALIGN - 1) & ~(MULTIBOOT_TAG_ALIGN - 1);
    }
}

static inline void pmm_set_bit(uint32_t page) {
    pmm_bitmap[page / 32] |=  (1u << (page % 32));
}

static inline void pmm_clear_bit(uint32_t page) {
    pmm_bitmap[page / 32] &= ~(1u << (page % 32));
}

static inline int pmm_test_bit(uint32_t page) {
    return (pmm_bitmap[page / 32] >> (page % 32)) & 1u;
}

void pmm_init(void)
{
	uint64_t max_addr = 0;
	for(size_t i = 0; i < usable_count; i++)
	{
		uint64_t end = usable_memory[i].base + usable_memory[i].length;
		if(end > max_addr) max_addr = end;
	}

	pmm_page_count = (uint32_t)((max_addr + PAGE_SIZE - 1) / PAGE_SIZE);

	uint32_t bits = pmm_page_count;
	uint32_t dwords = (bits + 31) / 32;
	pmm_bitmap_size = dwords;

	uint32_t bitmap_start = (uint32_t)&kernel_end;
	bitmap_start = (bitmap_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	pmm_bitmap = (uint32_t*)bitmap_start;

	for(uint32_t i = 0; i < pmm_bitmap_size; i++) pmm_bitmap[i] = 0xFFFFFFFF;

	for(size_t i = 0; i < usable_count; i++)
	{
		uint32_t base = usable_memory[i].base;
		uint32_t length = usable_memory[i].length;
		uint32_t start_page = base / PAGE_SIZE;
		uint32_t end_page = (base + length) / PAGE_SIZE;

		for(uint32_t p = start_page; p < end_page; p++) pmm_clear_bit(p);
	}

	pmm_set_bit(0);

	uint32_t k_start_page = ((uint32_t)&kernel_start) / PAGE_SIZE;
	uint32_t k_end_page = ((uint32_t)&kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
	for(uint32_t i = k_start_page; i < k_end_page; i++) pmm_set_bit(i);

	uint32_t b_start_page = bitmap_start / PAGE_SIZE;
	uint32_t b_end_page = (bitmap_start + pmm_bitmap_size * sizeof(uint32_t) + PAGE_SIZE - 1) / PAGE_SIZE;
	for(uint32_t i = b_start_page; i < b_end_page; i++) pmm_set_bit(i);

	uint32_t h_start_page = ((uint32_t)&kernel_heap_start) / PAGE_SIZE;
	uint32_t h_end_page = ((uint32_t)&kernel_heap_end + PAGE_SIZE - 1) / PAGE_SIZE;
	for(uint32_t i = h_start_page; i < h_end_page; i++) pmm_set_bit(i);
}

uint32_t pmm_alloc_page(void)
{
	for(uint32_t i = 0; i < pmm_page_count; i++)
	{
		if(!pmm_test_bit(i))
		{
			pmm_set_bit(i);
			return i * PAGE_SIZE;
		}
	}
	return 0;
}

void pmm_free_page(uint32_t addr)
{
	uint32_t page = addr / PAGE_SIZE;
	pmm_clear_bit(page);
}

void kheap_init() {
    kheap_head = (kheap_block_t*)&kernel_heap_start;
	uint32_t heap_size = (uint32_t)&kernel_heap_end - (uint32_t)&kernel_heap_start;

	kheap_head->size = heap_size - sizeof(kheap_block_t);
	kheap_head->free = 1;
	kheap_head->next = NULL;
}

void* kmalloc(uint32_t size) {
	if(size == 0) return NULL;

    // Align to 8 bytes
    size = (size + 7) & ~7;

    kheap_block_t* cur = kheap_head;

    while(cur)
	{
		if(cur->free && cur->size >= size)
		{
			uint32_t remaining = cur->size - size;
			if(remaining > sizeof(kheap_block_t) + 8)
			{
				kheap_block_t* new_block = (kheap_block_t*)((uint8_t*)cur + sizeof(kheap_block_t) + size);
				new_block->size = remaining - sizeof(kheap_block_t);
				new_block->free = 1;
				new_block->next = cur->next;

				cur->size = size;
				cur->next = new_block;
			}

			cur->free = 0;
			return (uint8_t*)cur + sizeof(kheap_block_t);
		}

		cur = cur->next;
	}

    return NULL;
}

void kfree(void* ptr)
{
	if(!ptr) return;

	kheap_block_t* block = (kheap_block_t*)((uint8_t*)ptr - sizeof(kheap_block_t));
	block->free = 1;

	kheap_block_t* cur = kheap_head;
	while(cur)
	{
		if(cur->free)
		{
			while(cur->next && cur->next->free)
			{
				cur->size += sizeof(kheap_block_t) + cur->next->size;
				cur->next = cur->next->next;
			}
		}
		cur = cur->next;
	}
}

#endif