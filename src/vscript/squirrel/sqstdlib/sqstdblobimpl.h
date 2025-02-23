/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_BLOBIMPL_H_
#define _SQSTD_BLOBIMPL_H_

// @NMRiH - Felis: Limit single blob size to 16MB, all blobs to 32MB
#define NMRIH_SQ_BLOB_MAX_SIZE 16777216
#define NMRIH_SQ_BLOB_MAX_TOTAL_SIZE 33554432
extern SQUnsignedInteger g_SQBlobTotalAlloced;

struct SQBlob : public SQStream
{
    SQBlob(SQInteger size) {
        _size = size;
        _allocated = size;
        _buf = (unsigned char *)sq_malloc(size);
        memset(_buf, 0, _size);
        _ptr = 0;
        _owns = true;

        // @NMRiH - Felis
        g_SQBlobTotalAlloced += size;
    }
    virtual ~SQBlob() {
        sq_free(_buf, _allocated);

        // @NMRiH - Felis
        g_SQBlobTotalAlloced -= _allocated;
    }
    SQInteger Write(void *buffer, SQInteger size) {
        if(!CanAdvance(size)) {
            GrowBufOf(_ptr + size - _size);
        }
        memcpy(&_buf[_ptr], buffer, size);
        _ptr += size;
        return size;
    }
    SQInteger Read(void *buffer,SQInteger size) {
        SQInteger n = size;
        if(!CanAdvance(size)) {
            if((_size - _ptr) > 0)
                n = _size - _ptr;
            else return 0;
        }
        memcpy(buffer, &_buf[_ptr], n);
        _ptr += n;
        return n;
    }
    bool Resize(SQInteger n) {
        if(!_owns) return false;

        // @NMRiH - Felis: Enforce limit
        if (n > NMRIH_SQ_BLOB_MAX_SIZE)
            return false;

        if(n != _allocated) {

            // @NMRiH - Felis: See if resizing would exhaust the total limit
            if (((g_SQBlobTotalAlloced - _allocated) + n) > NMRIH_SQ_BLOB_MAX_TOTAL_SIZE)
                return false;

            unsigned char *newbuf = (unsigned char *)sq_malloc(n);
            memset(newbuf,0,n);
            if(_size > n)
                memcpy(newbuf,_buf,n);
            else
                memcpy(newbuf,_buf,_size);
            sq_free(_buf,_allocated);

            // @NMRiH - Felis: Refresh total usage
            g_SQBlobTotalAlloced -= _allocated;
            g_SQBlobTotalAlloced += n;

            _buf=newbuf;
            _allocated = n;
            if(_size > _allocated)
                _size = _allocated;
            if(_ptr > _allocated)
                _ptr = _allocated;
        }
        return true;
    }
    bool GrowBufOf(SQInteger n)
    {
        bool ret = true;
        if(_size + n > _allocated) {
            if(_size + n > _size * 2)
                ret = Resize(_size + n);
            else
                ret = Resize(_size * 2);
        }
        _size = _size + n;
        return ret;
    }
    bool CanAdvance(SQInteger n) {
        if(_ptr+n>_size)return false;
        return true;
    }
    SQInteger Seek(SQInteger offset, SQInteger origin) {
        switch(origin) {
            case SQ_SEEK_SET:
                if(offset > _size || offset < 0) return -1;
                _ptr = offset;
                break;
            case SQ_SEEK_CUR:
                if(_ptr + offset > _size || _ptr + offset < 0) return -1;
                _ptr += offset;
                break;
            case SQ_SEEK_END:
                if(_size + offset > _size || _size + offset < 0) return -1;
                _ptr = _size + offset;
                break;
            default: return -1;
        }
        return 0;
    }
    bool IsValid() {
        return _size == 0 || _buf?true:false;
    }
    bool EOS() {
        return _ptr == _size;
    }
    SQInteger Flush() { return 0; }
    SQInteger Tell() { return _ptr; }
    SQInteger Len() { return _size; }
    SQUserPointer GetBuf(){ return _buf; }
private:
    SQInteger _size;
    SQInteger _allocated;
    SQInteger _ptr;
    unsigned char *_buf;
    bool _owns;
};

#endif //_SQSTD_BLOBIMPL_H_
