#!/bin/sh
# Builds the pinned toolchains of tests/source-debug that no package provides
# (RD-035): Oscar64 and KickC (with the KickAssembler jar KickC pins) from
# their tags, into DIR, and writes DIR/bin/kickc and DIR/bin/kickass. cc65
# and 64tass come from the system's packages (Ubuntu 24.04: apt-get install
# cc65 64tass). Needs git, make, a C++ compiler, Java and Maven.
#
#   tests/source-debug/scripts/provision_toolchains.sh DIR
#   RD_OSCAR64=DIR/oscar64/bin/oscar64 RD_KICKC=DIR/bin/kickc RD_KICKASS=DIR/bin/kickass \
#       python3 -I tests/source-debug/scripts/build_fixtures.py --check
#
# The versions are the ones the toolchain files pin (oscar64.yaml, kickc.yaml).
set -eu
DIR=${1:?usage: provision_toolchains.sh DIR}
mkdir -p "$DIR/bin"
DIR=$(cd "$DIR" && pwd)

if [ ! -x "$DIR/oscar64/bin/oscar64" ]; then
    git clone --depth 1 --branch v1.32.273 https://github.com/drmortalwombat/oscar64 "$DIR/oscar64"
    make -C "$DIR/oscar64/make" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
fi

if [ ! -f "$DIR/kickc/target/kickc-release.jar" ]; then
    git clone --depth 1 --branch 0.8.6 https://gitlab.com/camelot/kickc "$DIR/kickc"
    (cd "$DIR/kickc" && mvn -q -B -DskipTests package)
fi
(cd "$DIR/kickc" && mvn -q -B dependency:build-classpath -Dmdep.outputFile="$DIR/kickc.classpath")

K="$DIR/kickc"
cat > "$DIR/bin/kickc" <<WRAP
#!/bin/sh
exec java -cp "$K/target/kickc-release.jar:\$(cat "$DIR/kickc.classpath")" dk.camelot64.kickc.KickC -F "$K/src/main/fragment" -I "$K/src/main/kc/include" -L "$K/src/main/kc/lib" -P "$K/src/main/kc/target" "\$@"
WRAP
cat > "$DIR/bin/kickass" <<WRAP
#!/bin/sh
exec java -cp "\$(cat "$DIR/kickc.classpath")" kickass.KickAssembler "\$@"
WRAP
chmod +x "$DIR/bin/kickc" "$DIR/bin/kickass"
"$DIR/oscar64/bin/oscar64" -v | head -1
"$DIR/bin/kickc" -V | head -1
