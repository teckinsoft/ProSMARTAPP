#pragma once
#include <string>
#include <vector>
#include <memory>

class TopoDS_Shape; // Forward declaration
class TCollection_AsciiString;
class IGESHandler_PIMPL; // Forward declaration
class gp_Pnt;
class gp_Dir;
class IGESHandler
{
private:
    //std::unique_ptr<TopoDS_Shape>  mShapeLeft, mShapeRight; // Pointer to hold the loaded shape
    //std::unique_ptr<TopoDS_Shape> mFusedShape;
                  
    std::unique_ptr<IGESHandler_PIMPL> mpIGESHandlerPimpl; // Private implementor

public:
    // Constructor
    IGESHandler();

    // Destructor
    ~IGESHandler();

    // Shape propeety
    /*TopoDS_Shape* Shape() {
        return mShapeLeft;
    }*/

	void ZoomIn();


	void ZoomOut();

    // Function to load an IGES file
    void LoadIGES(const std::string& filePath, int order=0);

    // Function to save an IGES file
    void SaveIGES(const std::string& filePath, int order=0);

    // Function to align the part to the XY plane with its length along the X-axis
    void AlignToXYPlane(int order=0);

    // Function to compute the thumbnail view matrix for WPF PictureBox
    //void ComputeThumbnailMatrix(float matrix[4][4]);
    
    //std::vector<unsigned char> RenderToPNG();

    /*std::vector<unsigned char> GeneratePixmap(const TopoDS_Shape& shape, int width, int height);*/

    std::vector<unsigned char> DumpInputShapes(const int width, const int height);
    std::vector<unsigned char> DumpFusedShape(const int width, const int height);

    void ScrewRotationAboutMidPart(TopoDS_Shape& shape, const gp_Pnt& pt, const gp_Dir& axis, double angleDegrees);

    void   PerformZoomAndRender(bool zoomIn);
    void RotatePartBy180AboutZAxis(int order);
    void Redraw();
    void UnionShapes();
    void SaveAsIGS(const std::string& filePath);
    bool HasMultipleConnectedComponents(const TopoDS_Shape& shape);
    bool IsPointOnAnySurface(const TopoDS_Shape& shape, const gp_Pnt& point, double tolerance);
    bool DoesVectorIntersectShape(const TopoDS_Shape& shape, const gp_Pnt& point, const gp_Dir& direction, gp_Pnt& intersectionPoint);
    void Mirror();
    void HandleIntersectingBoundingCurves(TopoDS_Shape& fusedShape, double tolerance);
    //double ShortestDistanceBetweenShapes(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2, gp_Pnt& pointOnShape1, gp_Pnt& pointOnShape2);
    //void ExtractLargestSolid(const TopoDS_Shape& shape, TopoDS_Shape& largestShape);
    //void HandleMultipleConnectedComponents(TopoDS_Shape& fusedShape);
};