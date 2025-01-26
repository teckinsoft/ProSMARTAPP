#include "ProSMARTMngd.h"
#include <msclr/marshal_cppstd.h>
#include "IGESHandler.h"
#include "OCCTHandlerMngd.h"

using namespace System;

namespace IGESWrapper
{
    IGESHandlerWrapper::IGESHandlerWrapper()
        : mIgesHandler(nullptr) // Initialize the shape pointer
    {
    }

    IGESHandlerWrapper::~IGESHandlerWrapper()
    {
        // Clean up the unmanaged TopoDS_Shape instance
        if (mIgesHandler != nullptr)
        {
            delete mIgesHandler;
            mIgesHandler = nullptr;
        }
    }
    void IGESHandlerWrapper::ZoomIn()
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }
        mIgesHandler->PerformZoomAndRender(true);
    }

    void IGESHandlerWrapper::ZoomOut()
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }
        mIgesHandler->PerformZoomAndRender(false);
    }

    void IGESHandlerWrapper::Initialize() {
        if (mIgesHandler == nullptr) {
            mIgesHandler = new IGESHandler();
        }
        /*else {
            throw gcnew InvalidOperationException("Handler is already initialized.");
        }*/
    }

    void IGESHandlerWrapper::LoadIGES(System::String^ filePath, int order)
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }

        std::string stdFilePath = msclr::interop::marshal_as<std::string>(filePath);
        mIgesHandler->LoadIGES(stdFilePath, order);
    }

    void IGESHandlerWrapper::SaveIGES(System::String^ filePath, int order)
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("No mIgesHandler is loaded.");
        }

        std::string stdFilePath = msclr::interop::marshal_as<std::string>(filePath);
        mIgesHandler->SaveIGES(stdFilePath, order);
    }

    void IGESHandlerWrapper::AlignToXYPlane(int order)
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("No mIgesHandler is loaded.");
        }

        mIgesHandler->AlignToXYPlane(order);
    }
    void IGESHandlerWrapper::Redraw() {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }
        try
        {
            mIgesHandler->Redraw();
        }
        catch (const std::exception& ex) {
            // Convert native exception to managed exception
            throw gcnew System::Exception(gcnew System::String(ex.what()));
        }
    }
    void IGESHandlerWrapper::RotatePartBy180AboutZAxis(int order) {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }
        try
        {
            mIgesHandler->RotatePartBy180AboutZAxis(order);
            Redraw();
        }
        catch (const std::exception& ex) {
            // Convert native exception to managed exception
            throw gcnew System::Exception(gcnew System::String(ex.what()));
        }
    }

    array<unsigned char>^ IGESHandlerWrapper::DumpInputShapes(int width, int height)
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }

        try
        {
            // Call the native RenderToPNG method
            std::vector<unsigned char> pngData = mIgesHandler->DumpInputShapes(800,800);

            // Create a managed byte array and populate it
            array<unsigned char>^ managedPngData = gcnew array<unsigned char>(static_cast<int>(pngData.size()));
            for (size_t i = 0; i < pngData.size(); ++i)
            {
                managedPngData[i] = pngData[i];
            }

            return managedPngData;
        }
        catch (const std::exception& ex)
        {
            // Convert native exception to managed exception
            throw gcnew System::Exception(gcnew System::String(ex.what()));
        }
    }

    array<unsigned char>^ IGESHandlerWrapper::DumpFusedShape(int width, int height)
    {
       if (mIgesHandler == nullptr)
       {
          throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
       }

       try
       {
          // Call the native RenderToPNG method
          std::vector<unsigned char> pngData = mIgesHandler->DumpFusedShape(800, 800);

          // Create a managed byte array and populate it
          array<unsigned char>^ managedPngData = gcnew array<unsigned char>(static_cast<int>(pngData.size()));
          for (size_t i = 0; i < pngData.size(); ++i)
          {
             managedPngData[i] = pngData[i];
          }

          return managedPngData;
       }
       catch (const std::exception& ex)
       {
          // Convert native exception to managed exception
          throw gcnew System::Exception(gcnew System::String(ex.what()));
       }
    }

    void IGESHandlerWrapper::SaveAsIGS(System::String^ filePath)
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
        }

        try
        {
            // Convert managed System::String to std::string
            std::string stdFilePath = msclr::interop::marshal_as<std::string>(filePath);

            // Call the native SaveAsIGS method
            mIgesHandler->SaveAsIGS(stdFilePath);
        }
        catch (const std::exception& ex)
        {
            // Convert native exception to managed exception
            throw gcnew System::Exception(gcnew System::String(ex.what()));
        }
    }

    void IGESHandlerWrapper::UnionShapes()
{
    if (mIgesHandler == nullptr)
    {
        throw gcnew InvalidOperationException("Handler is not initialized. Call Initialize() first.");
    }

    try
    {
        // Call the native UnionShapes method
        mIgesHandler->UnionShapes();
    }
    catch (const std::exception& ex)
    {
        // Convert native exception to managed exception
        throw gcnew System::Exception(gcnew System::String(ex.what()));
    }
}


    /*array<float, 2>^ IGESHandlerWrapper::ComputeThumbnailMatrix()
    {
        if (mIgesHandler == nullptr)
        {
            throw gcnew InvalidOperationException("No mIgesHandler is loaded.");
        }

        float matrix[4][4];
        mIgesHandler->ComputeThumbnailMatrix(matrix);

        array<float, 2>^ managedMatrix = gcnew array<float, 2>(4, 4);
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                managedMatrix[i, j] = matrix[i][j];
            }
        }

        return managedMatrix;
    }*/
}
