#!/usr/bin/env python3
"""Source patches for dyld-1066.10's cache builder (run from the dyld source directory by .github/workflows/dyld-cache.yml).

  dsc-patch.py

1. No libobjc in this system: a dylib that merely carries an __objc_imageinfo section must not make the builder
   look for libobjc's protocol list ("Could not find libobjc"). With no /usr/lib/libobjc.A.dylib among the
   inputs the ObjC optimizer is switched off.
"""
import sys


def patch(path, old, new):
    s = open(path).read()
    if s.count(old) != 1:
        sys.exit('%s: expected exactly one occurrence of: %s' % (path, old.strip().split('\n')[0]))
    open(path, 'w').write(s.replace(old, new))


patch('cache_builder/NewSharedCacheBuilder.cpp',
      '''        if ( cacheDylib.inputMF->hasObjC() )
            this->objcOptimizer.objcDylibs.push_back(&cacheDylib);
    }
''',
      '''        if ( cacheDylib.inputMF->hasObjC() )
            this->objcOptimizer.objcDylibs.push_back(&cacheDylib);
    }
    {
        // patched for the BCM2711 system: no libobjc, so no ObjC optimization
        bool haveLibObjC = false;
        for ( CacheDylib* d : this->objcOptimizer.objcDylibs ) {
            if ( d->installName == "/usr/lib/libobjc.A.dylib" )
                haveLibObjC = true;
        }
        if ( !haveLibObjC )
            this->objcOptimizer.objcDylibs.clear();
    }
''')
