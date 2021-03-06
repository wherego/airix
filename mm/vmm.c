#include <mm/vmm.h>
#include <mm/pmm.h>
#include <kernel/klib.h>

static struct page_table * get_page_table(struct page_directory *page_dir,
                                          void *vaddr)
{
    uint32_t pde_index = VMM_PDE_INDEX(vaddr);
    return cast_p2v_or_null(page_dir->entries[pde_index] & 0xFFFFF000);
}

struct page_directory * vmm_alloc_vaddr_space()
{
    struct page_directory *page_dir =
        cast_p2v_or_null(pmm_alloc_page_address());

    if (page_dir)
    {
        for (uint32_t i = 0; i < NUM_PDE; ++i)
            page_dir->entries[i] = VMM_WRITABLE;
    }

    return page_dir;
}

void vmm_free_vaddr_space(struct page_directory *page_dir)
{
    physical_addr_t paddr = CAST_VIRTUAL_TO_PHYSICAL(page_dir);
    pmm_free_page_address(paddr);
}

struct page_table * vmm_alloc_page_table()
{
    struct page_table *page_tab =
        cast_p2v_or_null(pmm_alloc_page_address());

    if (page_tab)
    {
        for (uint32_t i = 0; i < NUM_PTE; ++i)
            page_tab->entries[i] = VMM_WRITABLE;
    }

    return page_tab;
}

void vmm_free_page_table(struct page_table *page_tab)
{
    physical_addr_t paddr = CAST_VIRTUAL_TO_PHYSICAL(page_tab);
    pmm_free_page_address(paddr);
}

void vmm_map_page_table_index(struct page_directory *page_dir, uint32_t index,
                              struct page_table *page_tab, uint32_t flag)
{
    physical_addr_t paddr = CAST_VIRTUAL_TO_PHYSICAL(page_tab);
    page_dir->entries[index] = (pde_t)paddr | flag | VMM_PRESENT;
}

struct page_table * vmm_unmap_page_table_index(struct page_directory *page_dir,
                                               uint32_t index, uint32_t flag)
{
    struct page_table *page_tab =
        cast_p2v_or_null(page_dir->entries[index] & 0xFFFFF000);
    page_dir->entries[index] = (flag & 0xFFF) & ~VMM_PRESENT;
    return page_tab;
}

struct page_table * vmm_get_page_table_index(struct page_directory *page_dir,
                                             uint32_t index, uint32_t *flag)
{
    if (flag) *flag = page_dir->entries[index] & 0xFFF;
    return cast_p2v_or_null(page_dir->entries[index] & 0xFFFFF000);
}

void vmm_map_page_index(struct page_table *page_tab, uint32_t index,
                        physical_addr_t paddr, uint32_t flag)
{
    page_tab->entries[index] = (pte_t)paddr | flag | VMM_PRESENT;
}

physical_addr_t vmm_unmap_page_index(struct page_table *page_tab,
                                     uint32_t index, uint32_t flag)
{
    physical_addr_t paddr = page_tab->entries[index] & 0xFFFFF000;
    page_tab->entries[index] = (flag & 0xFFF) & ~VMM_PRESENT;
    return paddr;
}

physical_addr_t vmm_get_page_index(struct page_table *page_tab,
                                   uint32_t index, uint32_t *flag)
{
    if (flag) *flag = page_tab->entries[index] & 0xFFF;
    return page_tab->entries[index] & 0xFFFFF000;
}

int vmm_map(struct page_directory *page_dir, void *vaddr,
            physical_addr_t paddr, uint32_t flag)
{
    int page = 0;
    struct page_table *page_tab = get_page_table(page_dir, vaddr);

    if (!page_tab)
    {
        page_tab = vmm_alloc_page_table();

        /* Out of memory */
        if (!page_tab) return -1;

        page = 1;
        vmm_map_page_table(page_dir, vaddr, page_tab, flag);
    }

    if (page_tab->entries[VMM_PTE_INDEX(vaddr)] & VMM_PRESENT)
        panic("Remap virtual address at %p.", vaddr);

    vmm_map_page(page_tab, vaddr, paddr, flag);
    return page;
}
