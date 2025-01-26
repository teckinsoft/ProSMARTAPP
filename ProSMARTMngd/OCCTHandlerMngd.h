#pragma once

#include <msclr/marshal_cppstd.h>
//#include "IGESHandler.h"
class IGESHandler;

namespace IGESWrapper
{
    public ref class IGESHandlerWrapper
    {
    private:
        IGESHandler* mIgesHandler;

    public:
        // Constructor to initialize the IGESHandlerWrapper
        IGESHandlerWrapper();

        // Destructor to clean up unmanaged resources
        ~IGESHandlerWrapper();
        
        void ZoomIn();
        
        void ZoomOut();
        // New method to explicitly initialize the wrapper
        void Initialize();

        // Load an IGES file
        void LoadIGES(System::String^ filePath, int order);

        // Save the IGES shape
        void SaveIGES(System::String^ filePath, int order);

        // Align the shape to the XY plane
        void AlignToXYPlane(int order);

        // Compute the thumbnail matrix for the shape
        //array<float, 2>^ ComputeThumbnailMatrix();

        array<unsigned char>^ DumpInputShapes(int width, int height);
        array<unsigned char>^ DumpFusedShape(int width, int height);
        void RotatePartBy180AboutZAxis(int order);
        void Redraw();
        void SaveAsIGS(System::String^ filePath);
        void UnionShapes();
    };
}
