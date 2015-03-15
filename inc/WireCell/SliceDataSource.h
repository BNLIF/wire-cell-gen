#ifndef WIRECELL_SLICEDATASOURCE_H
#define WIRECELL_SLICEDATASOURCE_H

#include "WireCell/FrameDataSource.h"
#include "WireCell/Slice.h"

namespace WireCell {

    /**
       SliceDataSource - deliver slices from a FrameDataSource
     */
    class SliceDataSource {
	WireCell::FrameDataSource& fds;
	int slicenum;
    public:
	SliceDataSource(WireCell::FrameDataSource& fds);
	virtual ~SliceDataSource();

	/// Return the number of slices in the current frame
	int size() const;

	/// Go to the given slice, return slice number or -1 on error
	int jump(int slice_number); 

	/// Go to the next slice, return its number or -1 on error
	int next();

	/// Get the current slice, returning its number
	int get(WireCell::Slice& slice);

    };

}

#endif
