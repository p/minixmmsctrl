# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /home/segin/minixmmsctrl/minixmmsctrl-0.1.ebuild,v 0.1 2005/12/14 01:33:13 segin Exp $

IUSE=""

DESCRIPTION="A really stripped version of xmmsctrl"
SRC_URI="http://segin.no-ip.org/mxc/${P}.tar.gz"
HOMEPAGE="http://segin.no-ip.org/mxc/"

SLOT="0"
LICENSE="GPL-2"
KEYWORDS="~alpha ~amd64 ~hppa ~ppc ~sparc ~x86"

DEPEND="media-sound/xmms"

src_unpack() {
	unpack ${A} || die
	cd ${S}
}

src_compile() {
	emake || die
}

src_install () {
	dobin minixmmsctrl
	dodoc README HELP

}
