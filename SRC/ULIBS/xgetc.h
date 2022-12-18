/* 
 * File:   xgetc.h
 * Author: pablo
 *
 * Created on 31 de octubre de 2022, 08:16 PM
 */

#ifndef XGETC_H
#define	XGETC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "frtos-io.h"
    
int xgetc( char *c );
int xfgetc( int fd, char *c );


#ifdef	__cplusplus
}
#endif

#endif	/* XGETC_H */

