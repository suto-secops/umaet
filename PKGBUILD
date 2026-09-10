# Maintainer: Suto <218235327+suto-secops@users.noreply.github.com>
pkgname=umaet-git
pkgver=1.0.0.r1.1d3da53
pkgrel=1
pkgdesc="A personal finance and budget tracking app for KDE Plasma"
arch=('x86_64' 'aarch64')
url="https://github.com/suto-secops/umaet"
license=('GPL-3.0-or-later')
depends=(
    'qt6-base'
    'qt6-declarative'
    'kirigami'
    'kcoreaddons'
    'ki18n'
    'sqlite'
)
makedepends=(
    'git'
    'cmake'
    'extra-cmake-modules'
    'ninja'
)
provides=('umaet')
conflicts=('umaet')
source=("git+https://github.com/suto-secops/umaet.git")
sha256sums=('SKIP')

pkgver() {
    cd "${pkgname%-git}"
    printf "1.0.0.r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
}

build() {
    cmake -B build -S "${pkgname%-git}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
