// Host driver for dyld's shared cache builder library (the MRM C interface of dyld-1066.10), replacing the stock
// dyld_shared_cache_builder.mm, which needs closed frameworks (Bom). Built by .github/workflows/dyld-cache.yml in
// place of that file.
//
//   dsc_driver OUTDIR ROOT LISTFILE [ARCH]
//
// LISTFILE has one install path per line (e.g. /usr/lib/libSystem.B.dylib); each is read from ROOT/<path> and given to
// the builder as a macOS dylib that must be in the cache; a line "exe PATH" adds an executable instead. Everything the builder produces (the cache, its
// sub-caches, .map, .symbols) is written to OUTDIR under its base name.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "mrm_shared_cache_builder.h"

static bool readFile(const std::string& path, std::vector<uint8_t>& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if ( f == nullptr )
        return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(n);
    bool ok = fread(out.data(), 1, n, f) == (size_t)n;
    fclose(f);
    return ok;
}

int main(int argc, const char* argv[])
{
    if ( argc < 4 ) {
        fprintf(stderr, "usage: %s OUTDIR ROOT LISTFILE [ARCH]\n", argv[0]);
        return 2;
    }
    std::string outDir = argv[1], root = argv[2];
    const char* arch = argc > 4 ? argv[4] : "arm64";

    uint32_t major = 0, minor = 0;
    getVersion(&major, &minor);
    fprintf(stderr, "builder interface %u.%u\n", major, minor);

    BuildOptions_v1 opts = {};
    opts.version             = 1;
    opts.updateName          = "pixnu";
    opts.deviceName          = "bcm2711";
    opts.disposition         = InternalDevelopment;
    opts.platform            = macOS;
    opts.archs               = &arch;
    opts.numArchs            = 1;
    opts.verboseDiagnostics  = true;
    opts.isLocallyBuiltCache = true;
    MRMSharedCacheBuilder* b = createSharedCacheBuilder(&opts);
    if ( b == nullptr ) {
        fprintf(stderr, "createSharedCacheBuilder failed\n");
        return 1;
    }

    std::ifstream list(argv[3]);
    std::string line;
    std::vector<std::vector<uint8_t>*> keep;
    int added = 0;
    while ( std::getline(list, line) ) {
        if ( line.empty() || line[0] == '#' )
            continue;
        auto* data = new std::vector<uint8_t>();
        keep.push_back(data);
        FileFlags flags = MustBeInCache;
        if ( line.compare(0, 4, "exe ") == 0 ) {
            line = line.substr(4);
            flags = NoFlags;
        }
        if ( !readFile(root + line, *data) ) {
            fprintf(stderr, "cannot read %s%s\n", root.c_str(), line.c_str());
            continue;
        }
        // lines "exe /usr/bin/x" are executables (the builder wants a main executable to build cache loaders against)
        if ( addFile(b, line.c_str(), data->data(), data->size(), flags) )
            ++added;
        else
            fprintf(stderr, "addFile rejected %s\n", line.c_str());
    }
    fprintf(stderr, "added %d files\n", added);

    bool ok = runSharedCacheBuilder(b);
    uint64_t n = 0;
    const char* const* errs = getErrors(b, &n);
    for ( uint64_t i = 0; i < n; ++i )
        fprintf(stderr, "builder error: %s\n", errs[i]);
    uint64_t cn = 0;
    const CacheResult* const* crs = getCacheResults(b, &cn);
    for ( uint64_t i = 0; i < cn; ++i ) {
        for ( uint64_t j = 0; j < crs[i]->numWarnings; ++j )
            fprintf(stderr, "warning: %s\n", crs[i]->warnings[j]);
        for ( uint64_t j = 0; j < crs[i]->numErrors; ++j )
            fprintf(stderr, "cache error: %s\n", crs[i]->errors[j]);
        fprintf(stderr, "cache %s uuid %s\n", crs[i]->deviceConfiguration ? crs[i]->deviceConfiguration : "?", crs[i]->uuidString ? crs[i]->uuidString : "?");
    }
    fprintf(stderr, "run: %s\n", ok ? "ok" : "failed");

    uint64_t fn = 0;
    const FileResult* const* frs = getFileResults(b, &fn);
    mkdir(outDir.c_str(), 0755);
    for ( uint64_t i = 0; i < fn; ++i ) {
        const FileResult_v1* r = (const FileResult_v1*)frs[i];
        std::string path = r->path;
        std::string base = path.substr(path.rfind('/') + 1);
        std::string dst = outDir + "/" + base;
        const uint8_t* data = r->data;
        uint64_t size = r->size;
        std::vector<uint8_t> tmp;
        if ( r->version >= 2 && data == nullptr ) {
            const FileResult_v2* r2 = (const FileResult_v2*)frs[i];
            if ( r2->tempFilePath != nullptr && readFile(r2->tempFilePath, tmp) ) {
                data = tmp.data();
                size = tmp.size();
            }
        }
        if ( data == nullptr ) {
            fprintf(stderr, "no data for %s\n", path.c_str());
            continue;
        }
        FILE* f = fopen(dst.c_str(), "wb");
        if ( f == nullptr || fwrite(data, 1, size, f) != size ) {
            fprintf(stderr, "cannot write %s\n", dst.c_str());
            return 1;
        }
        fclose(f);
        fprintf(stderr, "wrote %s (%llu bytes) for %s\n", dst.c_str(), (unsigned long long)size, path.c_str());
    }
    destroySharedCacheBuilder(b);
    return ok && fn > 0 ? 0 : 1;
}
