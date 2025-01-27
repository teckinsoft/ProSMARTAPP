#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <map>
#include <gp_Ax1.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Graphic3d_Mat4.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <TopoDS_Shape.hxx>
#include <Bnd_Box.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Image_AlienPixMap.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <WNT_Window.hxx>
#include <V3d_Viewer.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#include <GeomAPI_IntCS.hxx>
#include <Geom_Line.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomConvert.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GeomConvert_CompCurveToBSplineCurve.hxx>
#include <Standard_Handle.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Shell.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <GeomLProp_SurfaceTool.hxx>

#include "IGESHandler.h"








// Structure to hold face information with its length and width
struct SurfaceInfo {
   TopoDS_Face face;
   double length; // Longest dimension (Xmax - Xmin)
   double width;  // Shortest dimension
};

size_t GetBytesPerPixel(const Image_AlienPixMap& img)
{
   switch (img.Format()) {
   case Image_Format_RGB:  return 3;
   case Image_Format_RGBA: return 4;
   case Image_Format_BGR:  return 3;
   case Image_Format_BGRA: return 4;
   case Image_Format_Gray: return 1;
   default:
      throw std::runtime_error("Unsupported image format in AlienPixMap.");
   }
}


// Function to check if a face is a surface of revolution
bool isSurfaceOfRevolution(const TopoDS_Face& face) {
   Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
   if (surface.IsNull()) {
      return false;
   }

   // Check if the surface is a surface of revolution
   Handle(Geom_SurfaceOfRevolution) revSurface = Handle(Geom_SurfaceOfRevolution)::DownCast(surface);
   return !revSurface.IsNull();
}

// Function to compute the bounding box dimensions of a face
SurfaceInfo computeBoundingBox(const TopoDS_Face& face) {
   Bnd_Box bbox;
   BRepBndLib::Add(face, bbox);

   double xmin, ymin, zmin, xmax, ymax, zmax;
   bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

   SurfaceInfo info;
   info.face = face;
   info.length = xmax - xmin; // Longest dimension (X-axis)
   info.width = ymax - ymin;  // Shortest dimension (Y-axis)

   return info;
}


// Function to compute the shortest distance between two faces
double computeShortestDistance(const TopoDS_Face& face1, const TopoDS_Face& face2) {
   BRepExtrema_DistShapeShape distCalc(face1, face2);
   distCalc.Perform();

   if (!distCalc.IsDone()) {
      throw std::runtime_error("Distance calculation failed.");
   }

   return distCalc.Value(); // Returns the shortest distance
}

// Function to find two surface of revolutions with the longest lengths
std::pair<SurfaceInfo, SurfaceInfo> findClosestRevolutions(const TopoDS_Shape& shape, double tolerance = 1e-6) {
   std::vector<SurfaceInfo> revolutions;

   // Extract all faces from the shape
   for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
      TopoDS_Face face = TopoDS::Face(explorer.Current());

      // Check if the face is a surface of revolution
      if (isSurfaceOfRevolution(face)) {
         // Compute bounding box dimensions
         SurfaceInfo info = computeBoundingBox(face);
         revolutions.push_back(info);
      }
   }

   // Sort surfaces by their longest dimension (length)
   std::sort(revolutions.begin(), revolutions.end(), [](const SurfaceInfo& a, const SurfaceInfo& b) {
      return a.length > b.length;
      });

   // Ensure there are enough revolutions to process
   if (revolutions.size() < 2) {
      throw std::runtime_error("Not enough surfaces of revolution found.");
   }

   // Find the first pair with a shortest distance of zero (within tolerance)
   for (size_t i = 0; i < revolutions.size() - 1; ++i) {
      for (size_t j = i + 1; j < revolutions.size(); ++j) {
         const auto& rev1 = revolutions[i];
         const auto& rev2 = revolutions[j];

         // Compute the shortest distance between the two surfaces
         double distance = computeShortestDistance(rev1.face, rev2.face);

         // Check if the distance is within the tolerance
         if (distance <= tolerance) {
            return { rev1, rev2 }; // Return the first matching pair
         }
      }
   }

   throw std::runtime_error("No matching surfaces of revolution found within the given tolerance.");
}



// Function to sew two faces
TopoDS_Shape sewFaces(const TopoDS_Face& face1, const TopoDS_Face& face2, double tolerance) {
   BRepBuilderAPI_Sewing sewing(tolerance);

   // Add the faces to the sewing operation
   sewing.Add(face1);
   sewing.Add(face2);

   // Perform sewing
   sewing.Perform();

   return sewing.SewedShape();
}

// Main function to process and stitch two revolutions
TopoDS_Shape processRevolutions(const TopoDS_Shape& shape, double tolerance = 1e-6) {
   // Step 1: Find the two longest surface of revolutions
   auto [surf1, surf2] = findClosestRevolutions(shape);

   // Step 2: Check if they are aligned along their width
   if (std::abs(surf1.width - surf2.width) > tolerance) {
      throw std::runtime_error("The two surfaces are not aligned along their width.");
   }

   // Step 3: Sew the two surfaces together
   TopoDS_Shape sewnShape = sewFaces(surf1.face, surf2.face, tolerance);

   // Step 4: Unify the result (optional, to remove redundant edges)
   ShapeUpgrade_UnifySameDomain unify(sewnShape, Standard_True, Standard_True, Standard_False);
   unify.Build();

   return unify.Shape();
}

class IGESHandler_PIMPL {
   private:
   Handle(V3d_Viewer) viewer; // Open CASCADE viewer
   Handle(AIS_InteractiveContext) context; // AIS Context14
   BRepAlgoAPI_Fuse mFuser;
   TopoDS_Shape mShapeLeft, mShapeRight, mFusedShape;

   public:
   IGESHandler_PIMPL() = default;
   ~IGESHandler_PIMPL() {}

   // Constructor to initialize with a viewer
   explicit IGESHandler_PIMPL(const Handle(V3d_Viewer)& aViewer) : viewer(aViewer) {}

   void SetViewer(const Handle(V3d_Viewer)& aViewer) {
      viewer = aViewer;
   }

   void SetContext(const Handle(AIS_InteractiveContext)& aContext) {
      context = aContext;
   }

   Handle(V3d_Viewer) GetViewer() const {
      return viewer;
   }

   Handle(AIS_InteractiveContext) GetContext() const {
      return context;
   }

   V3d_ListOfView GetActiveViews() {
      return viewer->ActiveViews();
   }

   void SetFuser(const BRepAlgoAPI_Fuse& fuser) {
      mFuser = fuser;
   }

   BRepAlgoAPI_Fuse GetFuser() {
      return mFuser;
   }

   void SetLeftShape(const TopoDS_Shape& shape) {
      mShapeLeft = shape;
   }

   TopoDS_Shape GetLeftShape() {
      return mShapeLeft;
   }


   void SetRightShape(const TopoDS_Shape& shape) {
      mShapeRight = shape;
   }

   TopoDS_Shape GetRightShape() {
      return mShapeRight;
   }

   void SetFusedShape(const TopoDS_Shape& shape) {
      mFusedShape = shape;
   }

   TopoDS_Shape GetFusedShape() {
      return mFusedShape;
   }

   void SetMirroredShape(const TopoDS_Shape& shape) {
      mFusedShape = shape;
   }

   TopoDS_Shape GetMirroredShape() {
      return mFusedShape;
   }

   Bnd_Box GetBBox(const TopoDS_Shape& shape) {
      Bnd_Box bbox;
      BRepBndLib::Add(shape, bbox);
      double xmin, ymin, zmin, xmax, ymax, zmax;
      bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
      return bbox;
   }

   // Assuming bbox is a class with a Get method as described
   auto GetBBoxComp(const TopoDS_Shape& shape)
      -> std::tuple<double, double, double, double, double, double> {
      Bnd_Box bbox;
      BRepBndLib::Add(shape, bbox);
      double xmin, ymin, zmin, xmax, ymax, zmax;
      bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
      return std::make_tuple(xmin, ymin, zmin, xmax, ymax, zmax);
   }

   TopoDS_Shape TranslateAlongX(const TopoDS_Shape& shape, double requiredTranslation) {
      //double requiredTranslation = leftXmax - rightXmin - 0.4;
      //double requiredTranslation = delta;

      gp_Trsf moveRightTrsf;
      moveRightTrsf.SetTranslation(gp_Vec(requiredTranslation, 0, 0));

      // Apply the translation to mShapeRight
      BRepBuilderAPI_Transform moveRightTransform(shape, moveRightTrsf, true);
      return moveRightTransform.Shape();
   }

   double ShortestDistanceBetweenShapes(gp_Pnt& pointOnShape1, gp_Pnt& pointOnShape2) {
      // Create an instance of BRepExtrema_DistShapeShape
      BRepExtrema_DistShapeShape distanceCalculator(mShapeLeft, mShapeRight);

      // Check if the computation was successful
      if (!distanceCalculator.IsDone()) {
         throw std::runtime_error("Distance computation between shapes failed.");
      }

      // Get the minimum distance
      double minDistance = distanceCalculator.Value();

      // Retrieve the closest points on each shape
      if (distanceCalculator.NbSolution() > 0) {
         pointOnShape1 = distanceCalculator.PointOnShape1(1);
         pointOnShape2 = distanceCalculator.PointOnShape2(1);
      }

      return minDistance;
   }




   //// Function to check if a face is curved
   //bool isCurvedFace(const TopoDS_Face& face) {
   //   Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
   //   if (surface.IsNull()) {
   //      return false;
   //   }

   //   GeomAdaptor_Surface adaptor(surface);
   //   GeomAbs_SurfaceType surfaceType = adaptor.GetType();

   //   // Check if the surface type is curved (e.g., cylindrical, spherical, or NURBS)
   //   return (/*surfaceType == GeomAbs_Cylinder ||
   //      surfaceType == GeomAbs_Sphere ||
   //      surfaceType == GeomAbs_BSplineSurface ||
   //      surfaceType == GeomAbs_BezierSurface ||*/
   //      surfaceType == GeomAbs_SurfaceOfRevolution);
   //}

   //// Function to extract only curved faces from a shape
   //std::vector<TopoDS_Face> extractCurvedFaces(const TopoDS_Shape& shape) {
   //   std::vector<TopoDS_Face> curvedFaces;
   //   for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
   //      TopoDS_Face face = TopoDS::Face(explorer.Current());
   //      if (isCurvedFace(face)) {
   //         curvedFaces.push_back(face);
   //      }
   //   }
   //   return curvedFaces;
   //}

   //// Function to check if two curved faces are connected (topologically or by proximity)
   //bool areFacesConnected(const TopoDS_Face& face1, const TopoDS_Face& face2, double tolerance) {
   //   // Extract edges of both faces
   //   TopTools_IndexedMapOfShape edges1, edges2;
   //   TopExp::MapShapes(face1, TopAbs_EDGE, edges1);
   //   TopExp::MapShapes(face2, TopAbs_EDGE, edges2);

   //   // Check if any edges are shared or within the given tolerance
   //   for (int i = 1; i <= edges1.Extent(); i++) {
   //      const TopoDS_Edge& edge1 = TopoDS::Edge(edges1(i));
   //      for (int j = 1; j <= edges2.Extent(); j++) {
   //         const TopoDS_Edge& edge2 = TopoDS::Edge(edges2(j));
   //         if (edge1.IsEqual(edge2)) {
   //            return true; // Shared edge found
   //         }

   //         // Check proximity of the edges (use BRep_Tool to get edge geometry)
   //         Standard_Real f1, l1, f2, l2;
   //         Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edge1, f1, l1);
   //         Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edge2, f2, l2);

   //         if (!curve1.IsNull() && !curve2.IsNull()) {
   //            gp_Pnt start1 = curve1->Value(f1);
   //            gp_Pnt start2 = curve2->Value(f2);
   //            if (start1.Distance(start2) < tolerance) {
   //               return true;
   //            }
   //         }
   //      }
   //   }
   //   return false; // No connection found
   //}

   //// Function to group connected curved faces into clusters
   //std::vector<std::vector<TopoDS_Face>> groupConnectedCurvedFaces(const std::vector<TopoDS_Face>& faces, double tolerance) {
   //   std::vector<std::vector<TopoDS_Face>> clusters;
   //   std::map<int, bool> visited;

   //   for (size_t i = 0; i < faces.size(); ++i) {
   //      if (visited[i]) continue;

   //      std::vector<TopoDS_Face> cluster;
   //      std::vector<size_t> stack = { i }; // DFS stack
   //      while (!stack.empty()) {
   //         size_t current = stack.back();
   //         stack.pop_back();

   //         if (visited[current]) continue;
   //         visited[current] = true;

   //         cluster.push_back(faces[current]);

   //         // Find all connected curved faces
   //         for (size_t j = 0; j < faces.size(); ++j) {
   //            if (!visited[j] && areFacesConnected(faces[current], faces[j], tolerance)) {
   //               stack.push_back(j);
   //            }
   //         }
   //      }

   //      clusters.push_back(cluster);
   //   }
   //   return clusters;
   //}

   //// Function to sew and unify a cluster of curved faces
   //TopoDS_Shape sewAndUnifyCluster(const std::vector<TopoDS_Face>& cluster, double tolerance) {
   //   BRepBuilderAPI_Sewing sewing(tolerance);

   //   // Add all curved faces to the sewing operation
   //   for (const auto& face : cluster) {
   //      sewing.Add(face);
   //   }

   //   // Perform the sewing
   //   sewing.Perform();
   //   TopoDS_Shape sewnShape = sewing.SewedShape();

   //   // Unify adjacent faces with the same geometric domain
   //   ShapeUpgrade_UnifySameDomain unify(sewnShape, Standard_True, Standard_True, Standard_False);
   //   unify.Build();

   //   return unify.Shape();
   //}
   void SewFlexes(TopoDS_Shape& shape) {
      try {
         // Process and stitch the surfaces of revolution
         TopoDS_Shape resultShape = processRevolutions(shape);

         // Save the result
         //BRepTools::Write(resultShape, "stitchedRevolutions.brep");

         std::cout << "Successfully processed and stitched surfaces of revolution." << std::endl;
      }
      catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << std::endl;
      }
   }
   // Main function to process a shape
   //TopoDS_Shape processCurvedFaces(const TopoDS_Shape& inputShape, double tolerance = 1e-6) {
   //   // Step 1: Extract curved faces
   //   std::vector<TopoDS_Face> curvedFaces = extractCurvedFaces(inputShape);

   //   // Step 2: Group connected curved faces into clusters
   //   std::vector<std::vector<TopoDS_Face>> clusters = groupConnectedCurvedFaces(curvedFaces, tolerance);

   //   // Step 3: Process each cluster (sew and unify)
   //   TopoDS_Compound result;
   //   BRep_Builder builder;
   //   builder.MakeCompound(result);

   //   for (const auto& cluster : clusters) {
   //      TopoDS_Shape unifiedCluster = sewAndUnifyCluster(cluster, tolerance);
   //      builder.Add(result, unifiedCluster);
   //   }

   //   return result;
   //}

};




IGESHandler::IGESHandler()
// Initialize mShapeLeft pointer to nullptr
{
   mpIGESHandlerPimpl = std::make_unique< IGESHandler_PIMPL>();
}

IGESHandler::~IGESHandler()
{
   //// Cleanup the mShapeLeft pointer
   //if (mShapeLeft != nullptr)
   //{
   //    delete mShapeLeft;
   //    mShapeLeft = nullptr;
   //}
   //if (mShapeRight != nullptr)
   //{
   //    delete mShapeRight;
   //    mShapeRight = nullptr;
   //}
}

void IGESHandler::LoadIGES(const std::string& filePath, int order)
{
   try {
      IGESControl_Reader reader;
      if (!reader.ReadFile(filePath.c_str())) {
         throw std::runtime_error("Failed to read IGES file: " + filePath);
      }
      reader.TransferRoots();

      if (order == 0) {
         /*if (mShapeLeft != nullptr) {
             delete mShapeLeft;
         }*/
         //mShapeLeft = new TopoDS_Shape(reader.OneShape());
         mpIGESHandlerPimpl->SetLeftShape(TopoDS_Shape(reader.OneShape()));
         //return mShapeLeft;
      }
      else {
         /*if (mShapeRight != nullptr) {
             delete mShapeRight;
         }*/
         //mShapeRight = new TopoDS_Shape(reader.OneShape());
         //return mShapeRight;
         mpIGESHandlerPimpl->SetRightShape(TopoDS_Shape(reader.OneShape()));
      }


   }
   catch (const std::exception& ex) {
      std::cerr << "Exception in LoadIGES: " << ex.what() << std::endl;
      throw;
   }
}

void IGESHandler::SaveIGES(const std::string& filePath, int order)
{
   TopoDS_Shape shape;
   if (order == 0)  shape = mpIGESHandlerPimpl->GetLeftShape();
   else if (order == 1) shape = mpIGESHandlerPimpl->GetRightShape();
   else if (order == 2) shape = mpIGESHandlerPimpl->GetFusedShape();



   if (shape.IsNull())
   {
      throw std::runtime_error("No mShapeLeft is loaded to save.");
   }
   IGESControl_Writer writer;
   writer.AddShape(shape);
   if (!writer.Write(filePath.c_str()))
   {
      throw std::runtime_error("Failed to write IGES file: " + filePath);
   }
   /*TopoDS_Shape** shapePtr = nullptr;
   if (order == 0)  shapePtr = &mShapeLeft;
   else if (order == 1) shapePtr = &mShapeRight;
   else if (order == 2) {
       TopoDS_Shape* somePtr = mFusedShape.get();
       shapePtr = &somePtr;
   }

   if (shapePtr   == nullptr || (shapePtr  != nullptr && *shapePtr == nullptr))
   {
       throw std::runtime_error("No mShapeLeft is loaded to save.");
   }
   IGESControl_Writer writer;
   writer.AddShape(**shapePtr);
   if (!writer.Write(filePath.c_str()))
   {
       throw std::runtime_error("Failed to write IGES file: " + filePath);
   }*/
}

bool IGESHandler::HasMultipleConnectedComponents(const TopoDS_Shape& shape) {
   int solidCount = 0;

   // Traverse the shape to count solids
   for (TopExp_Explorer explorer(shape, TopAbs_SOLID); explorer.More(); explorer.Next()) {
      solidCount++;
      if (solidCount > 1) {
         return true;  // More than one solid found
      }
   }

   return false;  // Only one or no solid found
}


void IGESHandler::Redraw() {
   if (mpIGESHandlerPimpl->GetViewer().IsNull()) {
      throw std::runtime_error("Viewer is not set in the viewer implementation.");
   }

   Handle(V3d_View) view = mpIGESHandlerPimpl->GetActiveViews().First();
   if (view.IsNull()) {
      throw std::runtime_error("Active view is not initialized.");
   }

   // Refresh the viewer to update changes
   view->Redraw();
}

void IGESHandler::ZoomIn()
{
   if (!mpIGESHandlerPimpl) {
      throw std::runtime_error("Viewer is not initialized.");
   }

   if (mpIGESHandlerPimpl->GetViewer().IsNull()) {
      throw std::runtime_error("Viewer is not set in the viewer implementation.");
   }

   Handle(V3d_View) view = mpIGESHandlerPimpl->GetActiveViews().First();
   if (view.IsNull()) {
      throw std::runtime_error("Active view is not initialized.");
   }

   // Get the view dimensions
   Standard_Integer width, height;
   view->Window()->Size(width, height);

   // Define zoom rectangle near the center
   Standard_Integer centerX = width / 2;
   Standard_Integer centerY = height / 2;
   Standard_Integer delta = 20; // Adjust for zoom intensity

   // Perform zoom in
   view->Zoom(centerX - delta, centerY - delta, centerX + delta, centerY + delta);

   // Refresh the viewer to update changes
   view->Redraw();
}

void IGESHandler::ZoomOut()
{
   if (!mpIGESHandlerPimpl) {
      throw std::runtime_error("Viewer is not initialized.");
   }

   if (mpIGESHandlerPimpl->GetViewer().IsNull()) {
      throw std::runtime_error("Viewer is not set in the viewer implementation.");
   }

   Handle(V3d_View) view = mpIGESHandlerPimpl->GetActiveViews().First();
   if (view.IsNull()) {
      throw std::runtime_error("Active view is not initialized.");
   }

   // Get the view dimensions
   Standard_Integer width, height;
   view->Window()->Size(width, height);

   // Define zoom rectangle near the center
   Standard_Integer centerX = width / 2;
   Standard_Integer centerY = height / 2;
   Standard_Integer delta = 20; // Adjust for zoom intensity

   // Perform zoom out (larger rectangle)
   view->Zoom(centerX - delta * 2, centerY - delta * 2, centerX + delta * 2, centerY + delta * 2);

   // Refresh the viewer to update changes
   view->Redraw();
}

void  IGESHandler::RotatePartBy180AboutZAxis(int order) {

   TopoDS_Shape shape;
   if (order == 0) shape = mpIGESHandlerPimpl->GetLeftShape();
   else if (order == 1) shape = mpIGESHandlerPimpl->GetRightShape();
   if (shape.IsNull())
   {
      throw std::runtime_error("No mShapeLeft is loaded to apply screw rotation.");
   }
   // Compute the bounding box of the shape
   /*Bnd_Box bbox;
   BRepBndLib::Add(shape, bbox);
   bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);*/
   auto [xmin, ymin, zmin, xmax, ymax, zmax] = mpIGESHandlerPimpl->GetBBoxComp(shape);

   //double xmin, ymin, zmin, xmax, ymax, zmax;
   //bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

   // Calculate the midpoint of the bounding box in X and Y
   double xMid = (xmax + xmin) / 2.0;
   double yMid = (ymax + ymin) / 2.0;

   auto pt = gp_Pnt(xMid, yMid, 0);
   auto parallelaxis = gp_Dir(0, 0, 1);

   ScrewRotationAboutMidPart(shape, pt, parallelaxis, 180);
   if (order == 0) mpIGESHandlerPimpl->SetLeftShape(shape);
   else if (order == 1) mpIGESHandlerPimpl->SetRightShape(shape);
   AlignToXYPlane(order);
}

void IGESHandler::ScrewRotationAboutMidPart(TopoDS_Shape& shape, const gp_Pnt& pt, const gp_Dir& parallelaxis, double angleDegrees)
{
   /*TopoDS_Shape shape;
   if (order == 0) shape = mpIGESHandlerPimpl->GetLeftShape();
   else if ( order ==1 ) shape = mpIGESHandlerPimpl->GetRightShape();*/
   if (shape.IsNull())
   {
      throw std::runtime_error("No mShapeLeft is loaded to apply screw rotation.");
   }



   // Define the axis of rotation (parallel to Z-axis)
   gp_Ax1 rotationAxis(pt, parallelaxis);

   // Create the rotation transformation
   gp_Trsf rotationTrsf;
   rotationTrsf.SetRotation(rotationAxis, angleDegrees * M_PI / 180.0); // Convert degrees to radians

   // Apply the rotation to the shape
   BRepBuilderAPI_Transform rotator(shape, rotationTrsf, true);
   TopoDS_Shape rotatedShape = rotator.Shape();

   // Replace the original shape with the rotated shape
   //delete* shape;
   shape = TopoDS_Shape(rotatedShape);
}

bool IGESHandler::IsPointOnAnySurface(const TopoDS_Shape& shape, const gp_Pnt& point, double tolerance) {
   for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
      const TopoDS_Face& face = TopoDS::Face(explorer.Current());

      Handle(Geom_Surface) surface = BRep_Tool::Surface(face);

      if (!surface.IsNull()) {
         GeomAPI_ProjectPointOnSurf projector;
         projector.Init(point, surface);

         if (projector.NbPoints() > 0 && projector.LowerDistance() <= tolerance) {
            return true;
         }
      }
   }
   return false;
}


void IGESHandler::AlignToXYPlane(int order)
{
   TopoDS_Shape shape;
   if (order == 0) shape = mpIGESHandlerPimpl->GetLeftShape();
   else if (order == 1) shape = mpIGESHandlerPimpl->GetRightShape();

   if (shape.IsNull())
   {
      if (order == 0)
         throw std::runtime_error("No mShapeLeft is loaded to align.");
      else if (order == 1)
         throw std::runtime_error("No mShapeRight is loaded to align.");
   }

   // Compute the bounding box of the shape
   /*Bnd_Box bbox;
   BRepBndLib::Add(shape, bbox);
   double xmin, ymin, zmin, xmax, ymax, zmax;
   bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);*/
   double xmin, ymin, zmin, xmax, ymax, zmax;
   std::tie(xmin, ymin, zmin, xmax, ymax, zmax) = mpIGESHandlerPimpl->GetBBoxComp(shape);

   // Calculate dimensions
   double length = xmax - xmin;
   double width = ymax - ymin;
   double height = zmax - zmin;

   // Determine primary axes based on dimensions
   gp_Dir xAxis(1, 0, 0);
   gp_Dir yAxis(0, 1, 0);
   gp_Dir zAxis(0, 0, 1);

   if (length < width)
      std::swap(length, width), std::swap(xAxis, yAxis);
   if (length < height)
      std::swap(length, height), std::swap(xAxis, zAxis);
   if (width < height)
      std::swap(width, height), std::swap(yAxis, zAxis);

   // Align to the required orientation
   gp_Ax3 targetSystem(gp_Pnt(0, 0, 0), zAxis, xAxis); // Z-axis up, X-axis along longest dimension
   gp_Trsf alignmentTrsf;
   alignmentTrsf.SetTransformation(targetSystem, gp_Ax3(gp::Origin(), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0)));
   BRepBuilderAPI_Transform aligner(shape, alignmentTrsf, true);

   // Recalculate the bounding box after alignment
   TopoDS_Shape alignedShape = aligner.Shape();
   /*bbox.SetVoid();
   BRepBndLib::Add(alignedShape, bbox);
   bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);*/
   std::tie(xmin, ymin, zmin, xmax, ymax, zmax) = mpIGESHandlerPimpl->GetBBoxComp(alignedShape);

   // Calculate the translation required
   double yMid = (ymax + ymin) / 2.0;
   gp_Vec translation(xmin, yMid, zmin); // Translation vector to position the shape
   translation *= -1; // Move to required global positions

   gp_Trsf translationTrsf;
   translationTrsf.SetTranslation(translation);

   // Apply the translation
   BRepBuilderAPI_Transform finalTransform(alignedShape, translationTrsf, true);

   // Testing the bounds
   // Recalculate the bounding box after alignment
   shape = finalTransform.Shape();


   /*bbox.SetVoid();
   BRepBndLib::Add(shape, bbox);
   bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);*/
   std::tie(xmin, ymin, zmin, xmax, ymax, zmax) = mpIGESHandlerPimpl->GetBBoxComp(shape);
   auto xmid = (xmin + xmax) / 2.0;
   auto ymid = (ymin + ymax) / 2.0;
   auto zmid = (zmin + zmax) / 2.0;

   gp_Pnt fromPt(xmid, ymid, zmid);
   gp_Dir fromPtDirNegZ(0, 0, -1);
   gp_Pnt ixnPt;
   if (DoesVectorIntersectShape(shape, fromPt, fromPtDirNegZ, ixnPt)) {
      auto xAxis = gp_Dir(1, 0, 0);
      ScrewRotationAboutMidPart(shape, fromPt, xAxis, 180);
   }

   /*auto pt = gp_Pnt(xmin, ymid, zmin);
   if (IsPointOnAnySurface(shapePtr, pt, 1e-3)) {
      pt = gp_Pnt(xmin, ymid, zmid);
      auto axis = gp_Dir(1, 0, 0);
      ScrewRotationAboutMidPart(order, pt, axis, 180);
   }
    pt = gp_Pnt(xmin, ymid, zmax);
    if (IsPointOnAnySurface(shapePtr, pt, 1e-3)) {
        int aa = 0;
        ++aa;
    }*/


    // Replace the current shape with the aligned and positioned shape
    //delete* shapePtr;
    //shape = TopoDS_Shape(finalTransform.Shape());
   if (order == 0) mpIGESHandlerPimpl->SetLeftShape(shape);
   else if (order == 1) mpIGESHandlerPimpl->SetRightShape(shape);
   //*shapePtr = ptr;

   //// Align mShapeRight relative to mShapeLeft if both are present
   //if (!mpIGESHandlerPimpl->GetLeftShape().IsNull() && !mpIGESHandlerPimpl->GetRightShape().IsNull())
   //{
   //   auto leftShape = mpIGESHandlerPimpl->GetLeftShape();
   //   auto rightShape = mpIGESHandlerPimpl->GetRightShape();
   //   // Compute the bounding box of mShapeLeft
   //   /*Bnd_Box leftBox;
   //   BRepBndLib::Add(leftShape, leftBox);
   //   double leftXmin, leftYmin, leftZmin, leftXmax, leftYmax, leftZmax;
   //   leftBox.Get(leftXmin, leftYmin, leftZmin, leftXmax, leftYmax, leftZmax);*/
   //   double leftXmin, leftYmin, leftZmin, leftXmax, leftYmax, leftZmax;
   //   double rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax;
   //   std::tie(leftXmin, leftYmin, leftZmin, leftXmax, leftYmax, leftZmax) = mpIGESHandlerPimpl->GetBBoxComp(leftShape);

   //   // Compute the bounding box of mShapeRight
   //   /*Bnd_Box rightBox;
   //   BRepBndLib::Add(rightShape, rightBox);
   //   double rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax;
   //   rightBox.Get(rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax);*/
   //   std::tie( rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax) = mpIGESHandlerPimpl->GetBBoxComp(rightShape);

   //   rightShape = mpIGESHandlerPimpl->TranslateAlongX(rightShape, leftXmax - rightXmin - 0.4);

   //   //double requiredTranslation = leftXmax - rightXmin - 0.4;
   //   ////double requiredTranslation = delta;

   //   //gp_Trsf moveRightTrsf;
   //   //moveRightTrsf.SetTranslation(gp_Vec(requiredTranslation, 0, 0));

   //   //// Apply the translation to mShapeRight
   //   //BRepBuilderAPI_Transform moveRightTransform(rightShape, moveRightTrsf, true);
   //   //TopoDS_Shape movedShapeRight = moveRightTransform.Shape();

   //   //// Replace mShapeRight with the translated shape
   //   ////delete mShapeRight;
   //   //rightShape = TopoDS_Shape(movedShapeRight);
   //   mpIGESHandlerPimpl->SetRightShape(rightShape);
   //   std::tie(rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax) = mpIGESHandlerPimpl->GetBBoxComp(rightShape);

   //   //Bnd_Box rbox;
   //   //BRepBndLib::Add(rightShape, rbox);
   //   ////double rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax;
   //   //rbox.Get(rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax);

   //   //Bnd_Box lbox;
   //   //BRepBndLib::Add(leftShape, lbox);
   //   ////double rightXmin, rightYmin, rightZmin, rightXmax, rightYmax, rightZmax;
   //   //lbox.Get(leftXmin, leftYmin, leftZmin, leftXmax, leftYmax, leftZmax);
   //   std::tie(leftXmin, leftYmin, leftZmin, leftXmax, leftYmax, leftZmax) = mpIGESHandlerPimpl->GetBBoxComp(leftShape);

   //   // Calculate the required translation for mShapeRight
   //   gp_Pnt leftP(leftXmax, (leftYmin + leftYmax) / 2.0, leftZmax);
   //   gp_Pnt rightP(rightXmin, (rightYmin + rightYmax) / 2.0, rightZmax);
   //   // My code 
   //   auto delta = mpIGESHandlerPimpl->ShortestDistanceBetweenShapes(leftP, rightP);
   //   if (delta >= 0.4) {
   //      rightShape = mpIGESHandlerPimpl->TranslateAlongX(mpIGESHandlerPimpl->GetRightShape(), -delta);
   //      mpIGESHandlerPimpl->SetRightShape(rightShape);
   //   }

   //       // Re-check connectivity using the HasMultipleConnectedComponents method
   //   TopoDS_Compound compound;
   //   BRep_Builder builder;
   //   builder.MakeCompound(compound);
   //   builder.Add(compound, leftShape);
   //   builder.Add(compound, rightShape);

   //   /*if (HasMultipleConnectedComponents(compound)) {
   //      throw std::runtime_error("Shapes are not perfectly touching. Ensure bounding boxes are calculated correctly.");
   //   }*/
   //}

   //return *shapePtr;
}






bool IGESHandler::DoesVectorIntersectShape(const TopoDS_Shape& shape, const gp_Pnt& point, const gp_Dir& direction, gp_Pnt& intersectionPoint) {
   // Create a line from the point and direction
   Handle(Geom_Line) geomLine = new Geom_Line(point, direction);

   // Variable to store the closest intersection point and minimum distance
   gp_Pnt closestIntersection;
   double minDistanceSquared = std::numeric_limits<double>::max();
   bool intersectionFound = false;

   // Iterate over all faces in the shape
   for (TopExp_Explorer faceExplorer(shape, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next()) {
      const TopoDS_Face& face = TopoDS::Face(faceExplorer.Current());

      // Get the geometric surface of the face
      Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
      if (surface.IsNull()) {
         continue; // Skip invalid surfaces
      }

      // Check intersection of the line with the surface
      GeomAPI_IntCS intersectionChecker(geomLine, surface);

      // If there is an intersection
      if (intersectionChecker.IsDone() && intersectionChecker.NbPoints() > 0) {
         for (int i = 1; i <= intersectionChecker.NbPoints(); ++i) {
            gp_Pnt candidatePoint = intersectionChecker.Point(i);

            // Calculate the vector from the line origin to the intersection point
            gp_Vec toIntersection(point, candidatePoint);

            // Check if the vector aligns with the specified direction
            if (toIntersection * direction > 0) { // Dot product must be positive
               // Calculate the squared distance
               double distanceSquared = toIntersection.SquareMagnitude();

               // Update the closest point if this one is nearer
               if (distanceSquared < minDistanceSquared) {
                  minDistanceSquared = distanceSquared;
                  closestIntersection = candidatePoint;
                  intersectionFound = true;
               }
            }
         }
      }
   }

   // If an intersection was found, update the output parameter
   if (intersectionFound) {
      intersectionPoint = closestIntersection;
      return true;
   }

   // No valid intersection found
   return false;
}




//std::vector<unsigned char> IGESHandler::DumpInputShapes(const int width, const int height)
//{
//   std::vector<unsigned char> res;
//   auto leftShape = mpIGESHandlerPimpl->GetLeftShape();
//   auto mirroredShape = mpIGESHandlerPimpl->GetMirroredShape();
//   // Check if at least one shape is valid
//   if ((leftShape.IsNull()) &&
//      mirroredShape.IsNull()) {
//      throw std::runtime_error("Both shapes are null or not loaded.");
//   }
//
//   // Prepare viewer
//   Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
//   Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);
//
//   // Create a viewer and view
//   //mpViewerPimpl = std::make_unique<IGESHandler_Viewer>(Handle(V3d_Viewer)(new V3d_Viewer(graphicDriver)));
//   auto v3dViewer = (Handle(V3d_Viewer)(new V3d_Viewer(graphicDriver)));
//   mpIGESHandlerPimpl->SetViewer(v3dViewer);
//
//   //Handle(V3d_Viewer) viewer = new V3d_Viewer(graphicDriver);
//   mpIGESHandlerPimpl->GetViewer()->SetDefaultLights();
//   mpIGESHandlerPimpl->GetViewer()->SetLightOn();
//
//   // Prepare context
//   Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(mpIGESHandlerPimpl->GetViewer());
//
//   // Prepare off-screen view
//   Handle(V3d_View) view = mpIGESHandlerPimpl->GetViewer()->CreateView();
//   Handle(Aspect_NeutralWindow) wnd = new Aspect_NeutralWindow();
//   wnd->SetSize(width, height);
//   wnd->SetVirtual(true);
//   view->SetWindow(wnd);
//   view->SetBackgroundColor(Quantity_Color(Quantity_NOC_WHITE));
//   view->MustBeResized();
//
//   // Prepare bounding box for fitting
//   Bnd_Box combinedBoundingBox;
//
//   // Display mShapeLeft if available
//   if (!leftShape.IsNull()) {
//      Handle(AIS_Shape) leftPresentation = new AIS_Shape(leftShape);
//      context->Display(leftPresentation, Standard_False);
//      context->SetDisplayMode(leftPresentation, AIS_Shaded, Standard_False);
//      BRepBndLib::Add(leftShape, combinedBoundingBox);
//   }
//
//   // Display mMirroredShape if available
//   if (!mirroredShape.IsNull()) {
//      Handle(AIS_Shape) rightPresentation = new AIS_Shape(mirroredShape);
//      context->Display(rightPresentation, Standard_False);
//      context->SetDisplayMode(rightPresentation, AIS_Shaded, Standard_False);
//      BRepBndLib::Add(mirroredShape, combinedBoundingBox);
//   }
//
//   // Check if the bounding box is valid
//   if (combinedBoundingBox.IsVoid()) {
//      throw std::runtime_error("Bounding box of the shapes is void. Shapes might be empty.");
//   }
//
//   //// Fit view and redraw
//   //view->FitAll(0.01, Standard_True);
//   //view->Redraw();
//   // Fit view to the object and calculate the center
//   view->FitAll(0.01, Standard_True);
//   gp_Pnt bboxCenter;
//   if (!combinedBoundingBox.IsVoid()) {
//      Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
//      combinedBoundingBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
//      bboxCenter = gp_Pnt((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
//   }
//   else {
//      throw std::runtime_error("Bounding box is void. Cannot adjust view.");
//   }
//
//   // Adjust the viewpoint: move the camera closer
//   gp_Pnt eyePosition = bboxCenter.Translated(gp_Vec(0, 0, 300)); // Adjust the offset to move closer
//   view->SetEye(eyePosition.X(), eyePosition.Y(), eyePosition.Z());
//   view->SetAt(bboxCenter.X(), bboxCenter.Y(), bboxCenter.Z());
//
//   // Optionally, control zoom to focus further
//   view->SetZoom(1.5); // 1.5x zoom closer, adjust as needed
//
//   // Redraw the view
//   view->Redraw();
//
//   // Prepare pixmap image
//   Image_AlienPixMap img;
//   if (!view->ToPixMap(img, width, height)) {
//      throw std::runtime_error("Failed to render the view to pixmap.");
//   }
//
//   // Save image into a temporary file
//   TCollection_AsciiString filename = "C:\\temp\\iges_content.png";
//   img.Save(filename);
//
//   // Read the file content into memory
//   std::ifstream file(filename.ToCString(), std::ios::binary);
//   if (!file) {
//      throw std::runtime_error("Failed to open saved PNG file.");
//   }
//
//   std::vector<unsigned char> pngData((std::istreambuf_iterator<char>(file)),
//      std::istreambuf_iterator<char>());
//
//   // Delete the temporary file
//   std::remove(filename.ToCString());
//
//   return pngData;
//}

std::vector<unsigned char> IGESHandler::DumpInputShapes(const int width, const int height)
{
   try {
      std::vector<unsigned char> res;
      auto leftShape = mpIGESHandlerPimpl->GetLeftShape();
      auto mirroredShape = mpIGESHandlerPimpl->GetMirroredShape();

      if (leftShape.IsNull() && mirroredShape.IsNull()) {
         throw std::runtime_error("Both shapes are null or not loaded.");
      }

      // Initialize viewer
      Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
      Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);
      auto v3dViewer = (Handle(V3d_Viewer)(new V3d_Viewer(graphicDriver)));
      mpIGESHandlerPimpl->SetViewer(v3dViewer);

      mpIGESHandlerPimpl->GetViewer()->SetDefaultLights();
      mpIGESHandlerPimpl->GetViewer()->SetLightOn();
      Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(mpIGESHandlerPimpl->GetViewer());

      // Prepare off-screen view
      Handle(V3d_View) view = mpIGESHandlerPimpl->GetViewer()->CreateView();
      Handle(Aspect_NeutralWindow) wnd = new Aspect_NeutralWindow();
      wnd->SetSize(width, height);
      wnd->SetVirtual(true);
      view->SetWindow(wnd);
      view->SetBackgroundColor(Quantity_Color(Quantity_NOC_WHITE));
      view->MustBeResized();

      // Calculate bounding box
      Bnd_Box combinedBoundingBox;
      if (!leftShape.IsNull()) {
         Handle(AIS_Shape) leftPresentation = new AIS_Shape(leftShape);
         context->Display(leftPresentation, Standard_False);
         context->SetDisplayMode(leftPresentation, AIS_Shaded, Standard_False);
         BRepBndLib::Add(leftShape, combinedBoundingBox);
      }
      if (!mirroredShape.IsNull()) {
         Handle(AIS_Shape) rightPresentation = new AIS_Shape(mirroredShape);
         context->Display(rightPresentation, Standard_False);
         context->SetDisplayMode(rightPresentation, AIS_Shaded, Standard_False);
         BRepBndLib::Add(mirroredShape, combinedBoundingBox);
      }

      if (combinedBoundingBox.IsVoid()) {
         throw std::runtime_error("Bounding box of the shapes is void. Shapes might be empty.");
      }

      // Fit view and adjust camera
      view->FitAll(0.01, Standard_True);
      Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
      combinedBoundingBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
      gp_Pnt bboxCenter((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);

      gp_Vec offsetVec(0, 0, (xmax - xmin) * 0.5); // Dynamic offset
      gp_Pnt eyePosition = bboxCenter.Translated(offsetVec);
      view->SetEye(eyePosition.X(), eyePosition.Y(), eyePosition.Z());
      view->SetAt(bboxCenter.X(), bboxCenter.Y(), bboxCenter.Z());
      view->SetZoom(1.5);
      view->Redraw();

      // Capture pixmap
      Image_AlienPixMap img;
      if (!view->ToPixMap(img, width, height)) {
         throw std::runtime_error("Failed to render the view to pixmap.");
      }

      // Calculate the size of the pixmap data
      size_t bytesPerPixel = GetBytesPerPixel(img);
      size_t imgSize = img.Width() * img.Height() * bytesPerPixel;

      // Return the pixmap data as a vector of unsigned char
      return std::vector<unsigned char>(
         reinterpret_cast<const unsigned char*>(img.Data()),
         reinterpret_cast<const unsigned char*>(img.Data() + imgSize));
   }
   catch (const std::exception& ex) {
      std::cerr << "Error in DumpInputShapes: " << ex.what() << std::endl;
      throw;
   }
}



std::vector<unsigned char> IGESHandler::DumpFusedShape(const int width, const int height)
{
   std::vector<unsigned char> res;
   auto fusedShape = mpIGESHandlerPimpl->GetFusedShape();
   //auto mirroredShape = mpIGESHandlerPimpl->GetMirroredShape();
   // Check if at least one shape is valid
   if (fusedShape.IsNull())
      throw std::runtime_error("Both shapes are null or not loaded.");


   // Prepare viewer
   Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
   Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);

   // Create a viewer and view
   //mpViewerPimpl = std::make_unique<IGESHandler_Viewer>(Handle(V3d_Viewer)(new V3d_Viewer(graphicDriver)));
   auto v3dViewer = (Handle(V3d_Viewer)(new V3d_Viewer(graphicDriver)));
   mpIGESHandlerPimpl->SetViewer(v3dViewer);

   //Handle(V3d_Viewer) viewer = new V3d_Viewer(graphicDriver);
   mpIGESHandlerPimpl->GetViewer()->SetDefaultLights();
   mpIGESHandlerPimpl->GetViewer()->SetLightOn();

   // Prepare context
   Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(mpIGESHandlerPimpl->GetViewer());

   // Prepare off-screen view
   Handle(V3d_View) view = mpIGESHandlerPimpl->GetViewer()->CreateView();
   Handle(Aspect_NeutralWindow) wnd = new Aspect_NeutralWindow();
   wnd->SetSize(width, height);
   wnd->SetVirtual(true);
   view->SetWindow(wnd);
   view->SetBackgroundColor(Quantity_Color(Quantity_NOC_WHITE));
   view->MustBeResized();

   // Prepare bounding box for fitting
   Bnd_Box combinedBoundingBox;

   // Display mShapeLeft if available
   if (!fusedShape.IsNull()) {
      Handle(AIS_Shape) leftPresentation = new AIS_Shape(fusedShape);
      context->Display(leftPresentation, Standard_False);
      context->SetDisplayMode(leftPresentation, AIS_Shaded, Standard_False);
      BRepBndLib::Add(fusedShape, combinedBoundingBox);
   }

   // Check if the bounding box is valid
   if (combinedBoundingBox.IsVoid()) {
      throw std::runtime_error("Bounding box of the shapes is void. Shapes might be empty.");
   }

   // Fit view and redraw
   view->FitAll(0.01, Standard_True);
   view->Redraw();

   // Prepare pixmap image
   Image_AlienPixMap img;
   if (!view->ToPixMap(img, width, height)) {
      throw std::runtime_error("Failed to render the view to pixmap.");
   }

   // Save image into a temporary file
   TCollection_AsciiString filename = "C:\\temp\\iges_content.png";
   img.Save(filename);

   // Read the file content into memory
   std::ifstream file(filename.ToCString(), std::ios::binary);
   if (!file) {
      throw std::runtime_error("Failed to open saved PNG file.");
   }

   std::vector<unsigned char> pngData((std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());

   // Delete the temporary file
   std::remove(filename.ToCString());

   return pngData;
}

// Redraw and capture the updated image
void  IGESHandler::PerformZoomAndRender(bool zoomIn)
{
   if (zoomIn) {
      ZoomIn();
   }
   else {
      ZoomOut();
   }
}

//void IGESHandler::UnionShapes() {
//    try {
//        // Retrieve the shapes from the handler
//        auto leftShape = mpIGESHandlerPimpl->GetLeftShape();
//        auto rightShape = mpIGESHandlerPimpl->GetRightShape();
//
//        // Ensure both shapes are valid
//        if (leftShape.IsNull()) {
//            throw std::runtime_error("Left shape is null or not loaded.");
//        }
//        if (rightShape.IsNull()) {
//            throw std::runtime_error("Right shape is null or not loaded.");
//        }
//
//        // Check if both shapes are solids
//        if (leftShape.ShapeType() != TopAbs_SOLID || rightShape.ShapeType() != TopAbs_SOLID) {
//            throw std::runtime_error("Union operation requires both shapes to be solids.");
//        }
//
//        // Perform the union operation
//        BRepAlgoAPI_Fuse fuser(leftShape, rightShape);
//
//        // Explicitly build the fuse operation to ensure it's processed
//        fuser.Build();
//
//        // Validate the fuse operation
//        if (!fuser.IsDone()) {
//            throw std::runtime_error("Boolean union operation failed: Fuser did not complete.");
//        }
//
//        // Retrieve the fused shape
//        TopoDS_Shape fusedShape = fuser.Shape();
//        if (fusedShape.IsNull()) {
//            throw std::runtime_error("Fused shape is null after the union operation.");
//        }
//
//        // Store the fused shape and the fuser in the handler
//        mpIGESHandlerPimpl->SetFuser(fuser);
//        mpIGESHandlerPimpl->SetFusedShape(fusedShape);
//
//        // Optional: Validate the fused shape further (connected components)
//        
//        if (HasMultipleConnectedComponents(fusedShape)) {
//            throw std::runtime_error("Fused shape contains multiple connected components.");
//        }
//        
//    }
//    catch (const std::exception& ex) {
//        //std::cerr << "Error in UnionShapes: " << ex.what() << std::endl;
//        //std::string errormsg = "Error in UnionShapes : " + ex.what();
//        throw std::runtime_error(ex.what());
//    }
//    catch (...) {
//        std::cerr << "Unknown error occurred in UnionShapes." << std::endl;
//        throw std::runtime_error("Unknown error occurred in UnionShapes.");
//    }
//}
void IGESHandler::UnionShapes() {
   try {
      // Retrieve the shapes from the handler
      auto leftShape = mpIGESHandlerPimpl->GetLeftShape();
      Mirror();
      auto mirroredShape = mpIGESHandlerPimpl->GetMirroredShape();


      // Ensure both shapes are valid
      if (leftShape.IsNull()) {
         throw std::runtime_error("Left shape is null or not loaded.");
      }
      if (mirroredShape.IsNull()) {
         throw std::runtime_error("Right shape is null or not loaded.");
      }

      // Check if both shapes are solids
      if (leftShape.ShapeType() != TopAbs_SOLID || mirroredShape.ShapeType() != TopAbs_SOLID) {
         throw std::runtime_error("Union operation requires both shapes to be solids.");
      }

      // Perform the initial union operation
      BRepAlgoAPI_Fuse fuser(leftShape, mirroredShape);
      fuser.Build();

      // Validate the fuse operation
      if (!fuser.IsDone()) {
         throw std::runtime_error("Initial Boolean union operation failed.");
      }

      // Retrieve the initial fused shape
      TopoDS_Shape fusedShape = fuser.Shape();
      if (fusedShape.IsNull()) {
         throw std::runtime_error("Fused shape is null after the initial union operation.");
      }

      // Call the function to handle intersecting bounding curves
      double tolerance = 1e-2; // Adjust the tolerance as needed
      HandleIntersectingBoundingCurves(fusedShape, tolerance);

      //fusedShape = mpIGESHandlerPimpl->processCurvedFaces(fusedShape, tolerance = 1e-3);
      //mpIGESHandlerPimpl->SewFlexes(fusedShape);

      // Check for multiple connected components
      TopTools_IndexedMapOfShape solids;
      TopExp::MapShapes(fusedShape, TopAbs_SOLID, solids);

      // If there's more than one solid, merge them
      if (solids.Extent() > 1) {
         std::cout << "Multiple connected components detected. Performing iterative union." << std::endl;

         // Start with the first solid
         TopoDS_Shape unifiedSolid = solids(1);

         // Iteratively fuse the remaining solids
         for (int i = 2; i <= solids.Extent(); ++i) {
            BRepAlgoAPI_Fuse iterativeFuser(unifiedSolid, solids(i));
            iterativeFuser.Build();

            if (!iterativeFuser.IsDone()) {
               throw std::runtime_error("Iterative union operation failed.");
            }

            unifiedSolid = iterativeFuser.Shape();
         }

         // Update the fused shape to the unified result
         fusedShape = unifiedSolid;
      }

      // Store the final fused shape and fuser in the handler
      mpIGESHandlerPimpl->SetFuser(fuser); // Optionally store the last fuser
      mpIGESHandlerPimpl->SetFusedShape(fusedShape);

      // Optional: Validate the final fused shape
      BRepCheck_Analyzer analyzer(fusedShape);
      if (!analyzer.IsValid()) {
         throw std::runtime_error("Final fused shape is invalid.");
      }

      if (HasMultipleConnectedComponents(fusedShape)) {
         throw std::runtime_error("Fused shape contains multiple connected components.");
      }
      std::cout << "Boolean union operation completed successfully." << std::endl;

   }
   catch (const std::exception& ex) {
      std::cerr << "Error in UnionShapes: " << ex.what() << std::endl;
      throw std::runtime_error("More than 1 connected components found in boolean union");
   }
   catch (...) {
      std::cerr << "Unknown error occurred in UnionShapes." << std::endl;
      throw std::runtime_error("Unknown error occurred in UnionShapes.");
   }
}

void IGESHandler::Mirror() {
   // Retrieve the left shape
   auto leftShape = mpIGESHandlerPimpl->GetLeftShape();

   if (leftShape.IsNull()) {
      throw std::runtime_error("Left shape is null or not loaded.");
   }

   // Compute the bounding box of the left shape
   auto [xmin, ymin, zmin, xmax, ymax, zmax] = mpIGESHandlerPimpl->GetBBoxComp(leftShape);

   // Define the mirror plane passing through (xmax, ymax, zmax) with normal (-1, 0, 0)
   gp_Pnt planePoint(xmax, ymax, zmax);
   gp_Dir planeNormal(-1, 0, 0);
   gp_Ax2 mirrorPlane(planePoint, planeNormal);

   // Create a transformation for mirroring
   gp_Trsf mirrorTransformation;
   mirrorTransformation.SetMirror(mirrorPlane);

   // Apply the mirroring transformation to the left shape
   BRepBuilderAPI_Transform mirroringTransform(leftShape, mirrorTransformation, true);
   TopoDS_Shape mirroredShape = mirroringTransform.Shape();

   if (mirroredShape.IsNull()) {
      throw std::runtime_error("Failed to create mirrored shape.");
   }

   mirroredShape = mpIGESHandlerPimpl->TranslateAlongX(mirroredShape, -0.9);

   // Store the mirrored shape
   mpIGESHandlerPimpl->SetMirroredShape(mirroredShape);

   std::cout << "Mirroring operation completed successfully." << std::endl;
}


//void IGESHandler::HandleIntersectingBoundingCurves(TopoDS_Shape& fusedShape, double tolerance) {
//   // Extract edges from the fused shape
//   TopTools_IndexedMapOfShape edges;
//   TopExp::MapShapes(fusedShape, TopAbs_EDGE, edges);
//
//   // Store edges with their lengths
//   std::vector<std::pair<TopoDS_Edge, double>> edgeLengths;
//   for (int i = 1; i <= edges.Extent(); ++i) {
//      const TopoDS_Edge& edge = TopoDS::Edge(edges(i));
//      Standard_Real first, last;
//      Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
//      if (!curve.IsNull()) {
//         edgeLengths.emplace_back(edge, curve->Value(last).Distance(curve->Value(first)));
//      }
//   }
//
//   // Sort edges by length in descending order
//   std::sort(edgeLengths.begin(), edgeLengths.end(), [](const auto& a, const auto& b) {
//      return a.second > b.second;
//      });
//
//   // Take the top 5 lengthiest edges
//   size_t maxEdges = std::min(size_t(2), edgeLengths.size());
//   std::vector<TopoDS_Edge> topEdges;
//   for (size_t i = 0; i < maxEdges; ++i) {
//      topEdges.push_back(edgeLengths[i].first);
//   }
//
//   // Check intersections among the top edges only
//   for (size_t i = 0; i < topEdges.size(); ++i) {
//      const TopoDS_Edge& edge1 = topEdges[i];
//
//      for (size_t j = i + 1; j < topEdges.size(); ++j) {
//         const TopoDS_Edge& edge2 = topEdges[j];
//
//         // Check for intersection between edge1 and edge2
//         BRepExtrema_DistShapeShape dist(edge1, edge2);
//         if (dist.IsDone() && dist.Value() <= tolerance) {
//            // Extract the curves of the edges
//            Standard_Real first1, last1, first2, last2;
//            Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edge1, first1, last1);
//            Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edge2, first2, last2);
//
//            if (!curve1.IsNull() && !curve2.IsNull()) {
//               // Find the intersection parameter (approximate)
//               Standard_Real intersectionParam1 = (first1 + last1) / 2.0; // Adjust as needed
//               Standard_Real intersectionParam2 = (first2 + last2) / 2.0; // Adjust as needed
//
//               // Trim the curves
//               Handle(Geom_TrimmedCurve) trimmedCurve1 = new Geom_TrimmedCurve(curve1, first1, intersectionParam1);
//               Handle(Geom_TrimmedCurve) trimmedCurve2 = new Geom_TrimmedCurve(curve2, intersectionParam2, last2);
//
//               // Create edges for the trimmed curves
//               TopoDS_Edge trimmedEdge1 = BRepBuilderAPI_MakeEdge(trimmedCurve1);
//               TopoDS_Edge trimmedEdge2 = BRepBuilderAPI_MakeEdge(trimmedCurve2);
//
//               // Add the trimmed edges to a compound shape
//               TopoDS_Compound compound;
//               BRep_Builder builder;
//               builder.MakeCompound(compound);
//               builder.Add(compound, trimmedEdge1);
//               builder.Add(compound, trimmedEdge2);
//
//               // Replace the original edges in the fused shape
//               builder.Remove(fusedShape, edge1);
//               builder.Remove(fusedShape, edge2);
//               builder.Add(fusedShape, compound);
//            }
//         }
//      }
//   }
//
//   // Validate the modified fused shape
//   BRepCheck_Analyzer analyzer(fusedShape);
//   if (!analyzer.IsValid()) {
//      throw std::runtime_error("Modified fused shape is invalid.");
//   }
//
//   std::cout << "Intersecting bounding curves handled successfully with lazy evaluation." << std::endl;
//}

void IGESHandler::HandleIntersectingBoundingCurves(TopoDS_Shape& fusedShape, double tolerance) {

   //// Create the ShapeUpgrade_UnifySameDomain object
   //ShapeUpgrade_UnifySameDomain unify(fusedShape, Standard_True, Standard_True, Standard_False);

   //// Perform the unification
   //unify.Build();

   //// Get the refined shape
   //fusedShape = unify.Shape();

 // Step 1: Sew gaps between surfaces
   BRepBuilderAPI_Sewing sewing(tolerance);
   sewing.Add(fusedShape);
   sewing.Perform();
   TopoDS_Shape sewedShape = sewing.SewedShape();

   // Step 2: Fuse surfaces to create a single solid
   BRepAlgoAPI_Fuse fuse(sewedShape, sewedShape); // Self-fuse
   fuse.Build();
   fusedShape = fuse.Shape();

   // Step 3: Refine the shape to remove small edges
   ShapeUpgrade_UnifySameDomain unify(fusedShape, Standard_True, Standard_True, Standard_False);
   unify.Build();
   fusedShape = unify.Shape();

   // Step 1: Heal the shape to fix gaps and ensure continuity
   Handle(ShapeFix_Shape) shapeFix = new ShapeFix_Shape(fusedShape);
   shapeFix->SetPrecision(1e-3); // Set tolerance for fixing gaps
   shapeFix->Perform(); // Perform the healing operation
   TopoDS_Shape healedShape = shapeFix->Shape();

   // Step 2: Refine the healed shape
   unify = ShapeUpgrade_UnifySameDomain(healedShape, Standard_True, Standard_True, Standard_False);
   unify.Build();

   fusedShape = unify.Shape();
}

/*TopTools_IndexedMapOfShape edges, faces;
TopExp::MapShapes(fusedShape, TopAbs_EDGE, edges);
TopExp::MapShapes(fusedShape, TopAbs_FACE, faces);

std::vector<Bnd_Box> faceBoundingBoxes(faces.Extent());
for (int i = 1; i <= faces.Extent(); ++i) {
   const TopoDS_Face& face = TopoDS::Face(faces(i));
   BRepBndLib::Add(face, faceBoundingBoxes[i - 1]);
   faceBoundingBoxes[i - 1].Enlarge(tolerance);
}

TopoDS_Compound resultShape;
BRep_Builder resultBuilder;
resultBuilder.MakeCompound(resultShape);

#pragma omp parallel for schedule(static)
   for (int i = 1; i <= edges.Extent(); ++i) {
      const TopoDS_Edge& edge = TopoDS::Edge(edges(i));
      Bnd_Box edgeBox;
      BRepBndLib::Add(edge, edgeBox);
      edgeBox.Enlarge(tolerance);

      bool isTrimmed = false;
      for (int j = 0; j < faces.Extent(); ++j) {
         if (edgeBox.IsOut(faceBoundingBoxes[j])) {
            continue;
         }

         const TopoDS_Face& face = TopoDS::Face(faces(j + 1));
         BRepExtrema_DistShapeShape dist(edge, face);

         if (dist.IsDone() && dist.Value() <= tolerance) {
            isTrimmed = true;
            break;
         }
      }

#pragma omp critical
      {
         if (!isTrimmed) {
            resultBuilder.Add(resultShape, edge);
         }
      }
   }

   fusedShape = resultShape;

   BRepCheck_Analyzer analyzer(fusedShape);
   if (!analyzer.IsValid()) {
      throw std::runtime_error("Resulting shape is invalid.");
   }

   std::cout << "External boundary curves handled and trimmed successfully with static scheduling." << std::endl;
}*/









void IGESHandler::SaveAsIGS(const std::string& filePath) {
   // Check if mFusedShape is initialized
   if (mpIGESHandlerPimpl->GetFusedShape().IsNull()) {
      throw std::runtime_error("Fused shape is not initialized or empty.");
   }

   // Verify if mFusedShape has only one connected component
   TopTools_IndexedMapOfShape mapOfShapes;

   //auto fusedShape = mpIGESHandlerPimpl->GetFusedShape();
   TopExp::MapShapes(mpIGESHandlerPimpl->GetFusedShape(), TopAbs_SOLID, mapOfShapes);

   if (mapOfShapes.Extent() != 1) {
      throw std::runtime_error("Fused shape does not have exactly one connected component.");
   }

   // Write mFusedShape to an IGES file
   IGESControl_Writer writer;
   writer.AddShape(mpIGESHandlerPimpl->GetFusedShape());

   if (!writer.Write(filePath.c_str())) {
      throw std::runtime_error("Failed to write IGES file: " + filePath);
   }

   std::cout << "Successfully saved IGES file: " << filePath << std::endl;
}
