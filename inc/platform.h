/**
 * 
 */
#ifndef SMLT_PLATFORM_H_
#define SMLT_PLATFORM_H_ 1


/*
 * ===========================================================================
 * Barrelfish
 * ===========================================================================
 */
#if defined(BARRELFISH)
/// TODO: add platform specific includes


/*
 * ===========================================================================
 * Linux
 * ===========================================================================
 */
#elif defined(__linux__) || defined(__LINUX__)
/// TODO: add platform specific includes

#else
 /*
 * ===========================================================================
 * unsupported
 * ===========================================================================
 */
#error "unsupported OS"
#endif



struct sync_binding;
int _tree_prepare(void);
void _tree_export(const char *);
void _tree_register_receive_handlers(struct sync_binding *, void*);

void tree_connect(const char*);

#endif /* SMLT_PLATFORM_H_ */