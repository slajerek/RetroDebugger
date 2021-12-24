#ifndef DEVSID__H
#define DEVSID__H

/* The ioctl numbers are modelled after the hardsid linux device
   driver, so a software may autodetect which device is present behind
   a /dev/sid.
   http://hardsid.sourceforge.net/
 */

#define CWSID_IOCTL_TYPE ('S')

#define CWSID_IOCTL_RESET    _IO (CWSID_IOCTL_TYPE, 0)
#define CWSID_IOCTL_SIDTYPE  _IOR(CWSID_IOCTL_TYPE, 3, int)
#define CWSID_IOCTL_CARDTYPE _IOR(CWSID_IOCTL_TYPE, 4, int)

#define CWSID_IOCTL_WAITIRQ _IO(CWSID_IOCTL_TYPE, 0x10)
#define CWSID_IOCTL_PAL     _IO(CWSID_IOCTL_TYPE, 0x11)
#define CWSID_IOCTL_NTSC    _IO(CWSID_IOCTL_TYPE, 0x12)

#define CWSID_IOCTL_SETHZ        _IOW(CWSID_IOCTL_TYPE, 0x20,int)
#define CWSID_IOCTL_DOUBLEBUFFER _IOW(CWSID_IOCTL_TYPE, 0x21,int)
#define CWSID_IOCTL_DELAY        _IOW(CWSID_IOCTL_TYPE, 0x22,int)
#define CWSID_IOCTL_REALREAD     _IOW(CWSID_IOCTL_TYPE, 0x23,int)

/* a magic number used by the CWSID_IOCTL_SIDTYPE and
   CWSID_IOCTL_CARDTYPE ioctls to identify a CatWeasel SID */
#define CWSID_MAGIC 0x100

#endif
