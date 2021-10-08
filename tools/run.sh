#!/usr/bin/bash
if which gcc2vs > /dev/null; then
    export PIPECMD="2>&1 | gcc2vs"
fi
if [[ ! -z "$build_config" ]]; then
    cd "build/$build_config"
    echo Configuration: $build_config
fi
exec "$@"
