#!/bin/sh
#
# scripts/cutdown.sh
#
# Slim down source tree
#
# Copyright 2012 Sony Corporation
#
#  This program is free software; you can redistribute  it and/or modify it
#  under  the terms of  the GNU General  Public License as published by the
#  Free Software Foundation;  version 2 of the  License.
#
#  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
#  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
#  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
#  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
#  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
#  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#  You should have received a copy of the  GNU General Public License along
#  with this program; if not, write  to the Free Software Foundation, Inc.,
#  51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
#

# Wildcard matching is affected by LANG.
export LANG=C

# Delete tree except Kbuild, Kconfig and Makefile.
function cutdown
{
    if [ ! -e $1 ]; then
	return;
    fi
    /usr/bin/find $1 -type f -and ! \( -name "Kbuild" -o -name "Kconfig*" -o -name "Makefile*" \) -exec /bin/rm -f {} \;
    if [ ! -e $1 ]; then
	return;
    fi
    /usr/bin/find $1 -depth -type d -exec /bin/rmdir --ignore-fail-on-non-empty {} \;
}

(cd Documentation && \
    for i in B* IPMI* SM501* Submit* VGA* a[a-o]* aux* blackfin cdrom cris dcd* dell* device-mapper dvb eisa* frv g* hw_random* hwmon i2o ia64 ide infini* is* j* ko_KR ldm* led* logo* m68k mca* md* mips mn* par* powerpc s390 sgi* sh sparc*  svga* telephony video* x86 z*; \
    do
        cutdown $i; \
    done)
(cd Documentation/filesystems && \
    for i in 9* a[a-t]* b* cifs* coda* [g-h]* jfs* ncpfs* nilfs* ntfs* o* spu* sysv* ufs*; \
    do
        cutdown $i; \
    done)

(cd include && rm -rf acpi asm-alpha asm-arm26 asm-avr32 asm-[b-f]* asm-[h-z]* media)

(cd arch && rm -rf alpha avr32 [b-t]* unicore32 [v-z]*) #exclude um/
(cd arch && \
    for i in s390 um; \
    do \
        cutdown $i; \
    done)

(cd drivers && \
    for i in accessibility acpi atm auxdisplay bcma dio edac eisa firewire firmware hwmon infiniband isdn macintosh mca md media message mfd nubus parisc parport pnp ps3 rapidio s390 sbus sh sn tc telephony vlynq w1 zorro; \
    do \
        cutdown $i; \
    done)

(cd drivers/char && \
    for i in agp applicom* cyclades* dec* drm ds* epca* hv* hw_random ip2 ipmi mspec* mwave pcmcia snsc.* snsc_event.* sonypi* tpm xilinx_hwicap; \
    do \
        cutdown $i; \
    done)

(cd drivers/gpu && \
    for i in drm stub; \
    do \
        cutdown $i; \
    done)

(cd drivers/net && \
    for i in 3c* ace* appletalk arcnet atari* atl* benet bnx* can chelsio de* dgrs* e1000* ehea enic fs_enet hamradio hp* ibm* igb irda ix* mac* mlx4 myri* netxen ni* pasemi* pcmcia qla* sis* sk* spider* starfire* tlan* sun* tg* tokenring tulip typhoon* ucc* via*; \
    do \
        cutdown $i; \
    done)

(cd drivers/pcmcia && \
    for i in * \;
    do \
        cutdown $i; \
    done)

(cd drivers/scsi && \
    for i in [0-9]* [A-J]* [N-Z]* [a-b]* d* esp* g* ibm* in* j* libsas lpfc [m-q]* seagate.c sgi* sim* sni* sun* sym53* [t-z]*; \
    do \
        cutdown $i; \
    done)

(cd drivers/tty/serial && \
    for i in cpm_uart cris* dz* icom* ioc* ip* jsm* m32* mcf* mpsc* x* of* pmac* pnx* \;
    do \
        cutdown $i; \
    done)

(cd drivers/staging && \
    for i in a[a-m]* a[o-z]* [b-r]* s[a-s]* ste* [t-y]* \;
    do \
        cutdown $i; \
    done)

(cd drivers/video && \
    for i in aty geode i810 intelfb kyro matrox nvidia riva savage sis via; \
    do \
        cutdown $i; \
    done)

(cd firmware && \
    for i in *; \
    do \
        cutdown $i; \
    done)

(cd fs && \
    for i in 9p adfs affs afs befs bfs btrfs cifs coda efs freevxfs gfs2 hfs hfsplus hostfs hpfs hppfs jfs minix ncpfs ntfs ocfs2 openpromfs qnx4 reiserfs smbfs ubifs ufs; \
    do \
        cutdown $i; \
    done)

(cd fs/nls && \
    for i in nls_cp9*; \
    do \
        cutdown $i; \
    done)

(cd sound && \
    for i in aoa drivers i2c isa mips parisc pci pcmcia ppc soc sparc synth; \
    do \
        cutdown $i; \
    done)
(cd sound/oss && \
    for i in ac97* ad* aedp* au1550* bt* cs* emu* es* i810* msnd* nec* nm* opl* pas* pss* sb* sh* sscape* swarm* trident* trix* via* vwsnd* waveartist*
    do \
        cutdown $i; \
    done)

#/bin/rm -f setup-ebony setup-pc setup-tx49
