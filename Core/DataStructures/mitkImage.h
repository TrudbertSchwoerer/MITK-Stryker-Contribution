#ifndef MITKIMAGE_H_HEADER_INCLUDED_C1C2FCD2
#define MITKIMAGE_H_HEADER_INCLUDED_C1C2FCD2

#include "mitkCommon.h"
#include "SlicedData.h"
#include "DICOM.h"
#include "ImageDataItem.h"
#include "PixelType.h"
#include "BaseData.h"
#include "LevelWindow.h"

namespace mitk {

class SubImageSelector;

//##ModelId=3DCBC1E300FE
//##Documentation
//## Image class for storing images. Can be asked for header information, the
//## data vector, ipPicDescriptors or vtkImageData objects. If not the complete
//## data is required, the appropriate SubImageSelector class should be used
//## for access.
//## Image organizes sets of slices (s x 2D), volumes (t x 3D) and channels (n
//## x ND). Channels are for different kind of data (with the same Geometry!),
//## e.g., morphology in channel 0, velocities in channel 1.
class Image : public SlicedData
{
    friend class SubImageSelector;

  public:
	/** Standard class typedefs. */
    //##ModelId=3E0B6CF700ED
	typedef Image       Self;
    //##ModelId=3E0B6CF7015B
	typedef BaseData	Superclass;
    //##ModelId=3E0B6CF701AB
	typedef itk::SmartPointer<Self>  Pointer;
    //##ModelId=3E0B6CF70206
	typedef itk::SmartPointer<const Self>  ConstPointer;
	  
	/** Run-time type information (and related methods). */
	itkTypeMacro(Image,SlicedData);

    //##ModelId=3E18748C0045
    itkNewMacro(Self);  

	/** Smart Pointer type to a ImageDataItem. */
    //##ModelId=3E0B7B3C00F0
	typedef ImageDataItem::Pointer ImageDataItemPointer;

	/** STL Array of SmartPointers to ImageDataItems */
    //##ModelId=3E0B7B3C0172
	typedef std::vector<ImageDataItemPointer> ImageDataItemPointerArray;

  public:

	//##ModelId=3DCBC2B50345
    const mitk::PixelType& GetType(int n = 0) const;

    //##ModelId=3DCBC5AA0112
    unsigned int GetDimension() const;

    //##ModelId=3DCBC6040068
    unsigned int GetDimension(int i) const;

    //##ModelId=3DCBE46F026D
    virtual mitk::DICOM *GetDicom() const;


    //##ModelId=3E0B494802D6
    virtual void* GetData();

    //##ModelId=3DCBEF2902C6
    virtual vtkImageData* GetVtkImageData();

    //##ModelId=3DCBE2B802E4
    virtual ipPicDescriptor* GetPic();

	//##ModelId=3E1012990305
    virtual bool IsSliceSet(int s = 0, int t = 0, int n = 0) const;

    //##ModelId=3E10139001BF
    virtual bool IsVolumeSet(int t = 0, int n = 0) const;

    //##ModelId=3E1550E700E2
    virtual bool IsChannelSet(int n = 0) const;
  
    //##ModelId=3E10148003D7
    virtual bool SetSlice(void *data, int s = 0, int t = 0, int n = 0);

    //##ModelId=3E1014A00211
    virtual bool SetVolume(void *data, int t = 0, int n = 0);

    //##ModelId=3E1027F8023D
    //##Documentation
    //## set @a pic as slice @a s at time @a t in channel @a n. The
    //## corresponding @ Geomety2D is updated according to the information
    //## provided in the tags of @a pic.
    //## @return @a false : dimensions and/or data-type of @a pic does not
    //## comply with image 
    //## @a true success
    virtual bool SetPicSlice(ipPicDescriptor *pic, int s = 0, int t = 0, int n = 0);


    //##ModelId=3E102818024D
    //##Documentation
    //## set @a pic as volume at time @a t in channel @a n. The corresponding
    //## @a Geomety3D and depending @a Geometry2D entries are updated according
    //## to the information provided in the tags of @a pic.
    //## @return @a false : dimensions and/or data-type of @a pic does not
    //## comply with image 
    //## @a true success
    virtual bool SetPicVolume(ipPicDescriptor *pic, int t = 0, int n = 0);
    //##ModelId=3E102AE9004B
    //##Documentation
    //## initialize new (or re-initialize) image
    virtual void Initialize(const mitk::PixelType& type, unsigned int dimension, unsigned int *dimensions, unsigned int channels = 1);


    //##ModelId=3E102D2601DF
    //##Documentation
    //## initialize new (or re-initialize) image by @a pic. Dimensions and @a
    //## Geometry3D /@a Geometry2D  are set according to the tags in @a pic.
    //## @param tDim override time dimension (@a n[3]) in @a pic (if >0 and <)
    virtual void Initialize(ipPicDescriptor* pic, int channels = 1, int tDim = -1, int sDim = -1);
    //##ModelId=3E155CF000F6
    virtual bool IsValidSlice(int s = 0, int t = 0, int n = 0) const;


    //##ModelId=3E155D2501A7
    virtual bool IsValidVolume(int t = 0, int n = 0) const;
    //##ModelId=3E157C53030B
    virtual bool IsValidChannel(int n = 0) const;

    //##ModelId=3E19EA110396
	virtual bool IsInitialized() const;
    //##ModelId=3E1A11530384
    unsigned int* GetDimensions() const;

    //##Documentation
    //## @todo LevelWindow extraction/calculation not yet implemented. 
    virtual const mitk::LevelWindow& GetLevelWindow() const;

    //##Documentation
    //## @todo calculation of min/max not yet implemented. Timestamp must be added to track last calculation!
    virtual float GetScalarValueMin() const;

    //##Documentation
    //## @todo calculation of min/max not yet implemented. Timestamp must be added to track last calculation!
    virtual float GetScalarValueMax() const;
  protected:
    //##ModelId=3E0B4A6A01EC
    virtual mitk::ImageDataItem::Pointer GetSliceData(int s = 0, int t = 0, int n = 0);
    //##ModelId=3E0B4A82001A
    virtual mitk::ImageDataItem::Pointer GetVolumeData(int t = 0, int n = 0);
    //##ModelId=3E0B4A9100BC
    virtual mitk::ImageDataItem::Pointer GetChannelData(int n = 0);
    //##ModelId=3E155C940248
    int GetSliceIndex(int s = 0, int t = 0, int n = 0) const;


    //##ModelId=3E155C76012D
    int GetVolumeIndex(int t = 0, int n = 0) const;
    //##ModelId=3E155E7A0374
    void ComputeOffsetTable();
    //##ModelId=3E1569310328
    virtual mitk::ImageDataItem::Pointer AllocateSliceData(int s = 0, int t = 0, int n = 0);


    //##ModelId=3E15694500EC
    virtual mitk::ImageDataItem::Pointer AllocateVolumeData(int t = 0, int n = 0);

    //##ModelId=3E1569500322
    //##ModelId=3E1569500322
    virtual mitk::ImageDataItem::Pointer AllocateChannelData(int n = 0);
    //##ModelId=3E15F6C60103
    Image();
    //##ModelId=3E15F6CA014F
    ~Image();

    //##ModelId=3E0B7B3C0245
	mutable ImageDataItemPointerArray m_Channels;
    //##ModelId=3E0B7B3C0263
	mutable ImageDataItemPointerArray m_Volumes;
    //##ModelId=3E0B4C1C02C3
	mutable ImageDataItemPointerArray m_Slices;

    //##ModelId=3E156293019B
	unsigned int m_Dimension;
    //##ModelId=3E0B890702C4
	unsigned int *m_Dimensions;
    //##ModelId=3E0B8907036E
	unsigned int *m_OffsetTable;
    //##ModelId=3E10151F001F
    ImageDataItem::Pointer m_CompleteData;
    //##ModelId=3E13FE96005F
	PixelType m_PixelType;

    //##ModelId=3E19EA110292
	bool m_Initialized;

    mitk::LevelWindow m_LevelWindow;
};

} // namespace mitk

#endif /* MITKIMAGE_H_HEADER_INCLUDED_C1C2FCD2 */
