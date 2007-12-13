// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

namespace IceInternal
{

    //
    // An instance of ByteBuffer cannot grow beyond its initial capacity.
    // This class wraps a ByteBuffer and supports reallocation.
    //
    public class Buffer
    {
        public Buffer(int maxCapacity)
        {
            b = null;
            _size = 0;
            _capacity = 0;
            _maxCapacity = maxCapacity;
        }

        public int size()
        {
            return _size;
        }

        public bool empty()
        {
            return _size == 0;
        }

        public void clear()
        {
            b = null;
            _size = 0;
            _capacity = 0;
        }

        //
        // Call expand(n) to add room for n additional bytes. Note that expand()
        // examines the current position of the buffer first; we don't want to
        // expand the buffer if the caller is writing to a location that is
        // already in the buffer.
        //
        public void expand(int n)
        {
            int sz = b == null ? n : b.position() + n;
            if(sz > _size)
            {
                resize(sz, false);
            }
        }

        public void resize(int n, bool reading)
        {
            if(n == 0)
            {
                clear();
            }
            else if(n > _capacity)
            {
                reserve(n);
            }
            _size = n;

            //
            // When used for reading, we want to set the buffer's limit to the new size.
            //
            if(reading)
            {
                b.limit(_size);
            }
        }

        public void reset()
        {
            if(_size > 0 && _size * 2 < _capacity)
            {
                //
                // If the current buffer size is smaller than the
                // buffer capacity, we shrink the buffer memory to the
                // current size. This is to avoid holding on to too much
                // memory if it's not needed anymore.
                //
                if(++_shrinkCounter > 2)
                {
                    reserve(_size);
                    _shrinkCounter = 0;
                }
            }
            else
            {
                _shrinkCounter = 0;
            }
            _size = 0;
            if(b != null)
            {
                b.limit(b.capacity());
                b.position(0);
            }
        }

        private void reserve(int n)
        {
            if(n > _capacity)
            {
                _capacity = System.Math.Max(n, System.Math.Min(2 * _capacity, _maxCapacity));
                _capacity = System.Math.Max(240, _capacity);
            }
            else if(n < _capacity)
            {
                _capacity = n;
            }
            else
            {
                return;
            }

            try
            {
                ByteBuffer buf = ByteBuffer.allocate(_capacity);

                if(b == null)
                {
                    b = buf;
                }
                else
                {
                    int pos = b.position();
                    b.position(0);
                    b.limit(System.Math.Min(_capacity, b.capacity()));
                    buf.put(b);
                    b = buf;
                    b.limit(b.capacity());
                    b.position(pos);
                }

                b.order(ByteBuffer.ByteOrder.LITTLE_ENDIAN);
            }
            catch(System.OutOfMemoryException ex)
            {
                Ice.MarshalException e = new Ice.MarshalException(ex);
                e.reason = "OutOfMemoryException occurred while allocating a ByteBuffer";
                throw e;
            }
        }

        public ByteBuffer b;

        private int _size;
        private int _capacity; // Cache capacity to avoid excessive method calls.
        private int _maxCapacity;
        private int _shrinkCounter;
    }

}