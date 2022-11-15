#include "ccp.h"

// ---------------------------------------------------------------
void ccp_write_io(void *addr, uint8_t value)
{
	protected_write_io(addr, CCP_IOREG_gc, value);
}
// ---------------------------------------------------------------
void ccp_write_spm(void *addr, uint8_t value)
{
	protected_write_io(addr, CCP_SPM_gc, value);
}
// ---------------------------------------------------------------
