using IGESWrapper;
using System;
using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Imaging;

namespace ProSMARTAPP {
   public partial class MainWindow : Window {
      IGESHandlerWrapper igesHandler;

      Point mLastMousePosition; // For panning
      bool mIsPanning = false;  // Indicates panning state
      const double ZoomFactor = 1.1; // Zoom scale factor
      const double MinZoom = 0.5;    // Minimum zoom level
      const double MaxZoom = 5.0;    // Maximum zoom level

      public MainWindow () {
         InitializeComponent ();
      }

      void LoadPart (string filename, int order) {
         try {
            // Initialize and use IGESHandlerWrapper
            igesHandler ??= new IGESHandlerWrapper ();
            igesHandler.Initialize ();
            igesHandler.LoadIGES (filename, order);

            // Align to XY plane
            igesHandler.AlignToXYPlane (order);

            // Render the IGES to a PNG byte array
            byte[] pngData = igesHandler.DumpInputShapes (800, 600);

            // Save the file path in the appropriate TextBox
            if (order == 0) {
               Part1FileNameTextBox.Text = filename;
            } else {
               Part2FileNameTextBox.Text = filename;
            }

            // Display the PNG image in the ImageControl
            DisplayInputsImage ();
         } catch (Exception ex) {
            MessageBox.Show (ex.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
         }
      }

      async void OnPart1Click (object sender, RoutedEventArgs e) {
         var openFileDialog = new Microsoft.Win32.OpenFileDialog {
            Title = "Select a Part File",
            Filter = "CAD Files (*.iges;*.igs;*.stp;*.step)|*.iges;*.igs;*.stp;*.step|All Files (*.*)|*.*",
            Multiselect = false
         };

         if (openFileDialog.ShowDialog () == true) {
            string filePath = openFileDialog.FileName;

            try {
               // Set the busy cursor
               Mouse.OverrideCursor = Cursors.Wait;

               // Perform the long-running operation on a background thread
               await Task.Run (() => {
                  // Ensure UI operations are marshaled back to the UI thread
                  Application.Current.Dispatcher.Invoke (() => LoadPart (filePath, 0));
               });
            } catch (Exception ex) {
               // Handle exceptions if needed
               MessageBox.Show ($"An error occurred: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            } finally {
               // Reset the cursor
               Mouse.OverrideCursor = null;
            }
         } else {
            MessageBox.Show ("No file selected.", "File Open", MessageBoxButton.OK, MessageBoxImage.Information);
         }
      }

      async void OnPart2Click (object sender, RoutedEventArgs e) {
         var openFileDialog = new Microsoft.Win32.OpenFileDialog {
            Title = "Select a Part File",
            Filter = "CAD Files (*.iges;*.igs;*.stp;*.step)|*.iges;*.igs;*.stp;*.step|All Files (*.*)|*.*",
            Multiselect = false
         };

         if (openFileDialog.ShowDialog () == true) {
            string filePath = openFileDialog.FileName;

            try {
               // Set the busy cursor
               Mouse.OverrideCursor = Cursors.Wait;

               // Perform the long-running operation on a background thread
               await Task.Run (() => {
                  // Ensure UI operations are marshaled back to the UI thread
                  Application.Current.Dispatcher.Invoke (() => LoadPart (filePath, 1));
               });
            } catch (Exception ex) {
               // Handle exceptions if needed
               MessageBox.Show ($"An error occurred: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            } finally {
               // Reset the cursor
               Mouse.OverrideCursor = null;
            }
         } else {
            MessageBox.Show ("No file selected.", "File Open", MessageBoxButton.OK, MessageBoxImage.Information);
         }
      }

      async void OnPart1Rotate180Deg (object sender, RoutedEventArgs e) {
         try {
            // Set the busy cursor
            Mouse.OverrideCursor = Cursors.Wait;

            // Perform the long-running operation on a background thread
            await Task.Run (() => {
               // Ensure UI operations are marshaled back to the UI thread
               Application.Current.Dispatcher.Invoke (() => igesHandler.RotatePartBy180AboutZAxis (0));
            });
            DisplayInputsImage ();
         } catch (Exception ex) {
            // Handle exceptions if needed
            MessageBox.Show ($"An error occurred: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
         } finally {
            // Reset the cursor
            Mouse.OverrideCursor = null;
         }
      }

      async void OnPart2Rotate180Deg (object sender, RoutedEventArgs e) {
         try {
            // Set the busy cursor
            Mouse.OverrideCursor = Cursors.Wait;

            // Perform the long-running operation on a background thread
            await Task.Run (() => {
               // Ensure UI operations are marshaled back to the UI thread
               Application.Current.Dispatcher.Invoke (() => igesHandler.RotatePartBy180AboutZAxis (1));
            });
            DisplayInputsImage ();
         } catch (Exception ex) {
            // Handle exceptions if needed
            MessageBox.Show ($"An error occurred: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
         } finally {
            // Reset the cursor
            Mouse.OverrideCursor = null;
         }
      }

      void DisplayInputsImage () {
         if (igesHandler == null)
            throw new Exception ("IGES Handler is null");

         byte[] imageData = igesHandler.DumpInputShapes (800, 600);

         using var stream = new MemoryStream (imageData);
         var bitmap = new BitmapImage ();
         bitmap.BeginInit ();
         bitmap.CacheOption = BitmapCacheOption.OnLoad;
         bitmap.StreamSource = stream;
         bitmap.EndInit ();

         ImageControl.Source = bitmap;
      }
      void DisplayOutputImage () {
         if (igesHandler == null)
            throw new Exception ("IGES Handler is null");

         byte[] imageData = igesHandler.DumpFusedShape (800, 600);

         using var stream = new MemoryStream (imageData);
         var bitmap = new BitmapImage ();
         bitmap.BeginInit ();
         bitmap.CacheOption = BitmapCacheOption.OnLoad;
         bitmap.StreamSource = stream;
         bitmap.EndInit ();

         ImageControl.Source = bitmap;
      }

      void OnMouseWheel (object sender, MouseWheelEventArgs e) {
         try {
            if (igesHandler != null) {
               if (e.Delta > 0) {
                  igesHandler.ZoomIn ();
               } else if (e.Delta < 0) {
                  igesHandler.ZoomOut ();
               }
               //DisplayImage();
            }
         } catch (Exception ex) {
            MessageBox.Show ($"Zoom operation failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
         }
      }

      async void OnUnionClick (object sender, RoutedEventArgs e) {
         var saveFileDialog = new Microsoft.Win32.SaveFileDialog {
            Title = "Save Unioned IGES File",
            DefaultExt = ".igs",
            Filter = "IGES Files (*.igs)|*.igs|IGES Files (*.iges)|*.iges",
            InitialDirectory = @"W:\FChassis\Sample",
            FileName = "UnionResult.igs"
         };

         try {
            // Set the busy cursor
            Mouse.OverrideCursor = Cursors.Wait;

            byte[] unionImageData = null;

            // Perform the union operation on a background thread
            await Task.Run (() =>
            {
               // Perform the union operation
               igesHandler.UnionShapes ();

               // Generate the union image
               unionImageData = igesHandler.DumpFusedShape (800, 600);
            });

            // Display the union image on the UI thread
            if (unionImageData != null) {
               await Application.Current.Dispatcher.InvokeAsync (() =>
               {
                  using var ms = new MemoryStream (unionImageData);
                  var bitmap = new BitmapImage ();
                  bitmap.BeginInit ();
                  bitmap.CacheOption = BitmapCacheOption.OnLoad;
                  bitmap.StreamSource = ms;
                  bitmap.EndInit ();
                  ImageControl.Source = bitmap;
               });
            }

            // Show the file save dialog and save the IGES file
            if (saveFileDialog.ShowDialog () == true) {
               await Task.Run (() =>
               {
                  // Save the unioned shape to the selected file
                  igesHandler.SaveIGES (saveFileDialog.FileName, 2);
               });

               MessageBox.Show ($"Unioned IGES file saved successfully to {saveFileDialog.FileName}",
                               "Save Successful", MessageBoxButton.OK, MessageBoxImage.Information);
            }
         } catch (Exception ex) {
            MessageBox.Show ($"Error during union operation: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
         } finally {
            // Reset the cursor
            Mouse.OverrideCursor = null;
         }
      }

      //void OnUnionClick (object sender, RoutedEventArgs e) {
      //   try {
      //      // Perform union operation on the two parts
      //      igesHandler.UnionShapes ();

      //      // Generate the union image and display it
      //      var unionImageData = igesHandler.DumpFusedShape (800, 600);

      //      DisplayOutputImage ();
      //      // Display the generated PNG image
      //      using (var ms = new MemoryStream (unionImageData)) {
      //         var bitmap = new BitmapImage ();
      //         bitmap.BeginInit ();
      //         bitmap.CacheOption = BitmapCacheOption.OnLoad;
      //         bitmap.StreamSource = ms;
      //         bitmap.EndInit ();
      //         ImageControl.Source = bitmap;
      //      }

      //      // Open a File Save dialog to save the resulting IGES file
      //      var saveFileDialog = new Microsoft.Win32.SaveFileDialog {
      //         Title = "Save Unioned IGES File",
      //         DefaultExt = ".igs",
      //         Filter = "IGES Files (*.igs)|*.igs|IGES Files (*.iges)|*.iges",
      //         InitialDirectory = @"W:\FChassis\Sample",
      //         FileName = "UnionResult.igs"
      //      };

      //      if (saveFileDialog.ShowDialog () == true) {
      //         // Save the unioned shape to the selected file
      //         igesHandler.SaveIGES (saveFileDialog.FileName, 2); // Assuming order 0 is for the union result
      //         MessageBox.Show ($"Unioned IGES file saved successfully to {saveFileDialog.FileName}");
      //      }
      //   } catch (Exception ex) {
      //      MessageBox.Show ($"Error during union operation: {ex.Message}");
      //   }
      //}

      void ImageControl_MouseLeftButtonDown (object sender, MouseButtonEventArgs e) {
         mLastMousePosition = e.GetPosition (this); // Capture the mouse position
         mIsPanning = true; // Enable panning
         ImageControl.CaptureMouse (); // Capture the mouse for continued event handling
      }

      void ImageControl_MouseMove (object sender, MouseEventArgs e) {
         if (mIsPanning) {
            Point currentMousePosition = e.GetPosition (this); // Current mouse position
            double offsetX = currentMousePosition.X - mLastMousePosition.X; // Horizontal offset
            double offsetY = currentMousePosition.Y - mLastMousePosition.Y; // Vertical offset

            PanTransform.X += offsetX; // Apply horizontal panning
            PanTransform.Y += offsetY; // Apply vertical panning

            mLastMousePosition = currentMousePosition; // Update the last position
         }
      }

      void ImageControl_MouseLeftButtonUp (object sender, MouseButtonEventArgs e) {
         mIsPanning = false; // Disable panning
         ImageControl.ReleaseMouseCapture (); // Release the mouse capture
      }

      void ImageControl_MouseWheel (object sender, MouseWheelEventArgs e) {
         double zoomFactor = e.Delta > 0 ? 1.1 : 0.9; // Zoom in or out
         Point mousePosition = e.GetPosition (ImageControl); // Get mouse position relative to the image

         double newScaleX = ImageScale.ScaleX * zoomFactor;
         double newScaleY = ImageScale.ScaleY * zoomFactor;

         // Clamp the zoom levels
         if (newScaleX < MinZoom || newScaleX > MaxZoom)
            return;

         ImageScale.ScaleX = newScaleX;
         ImageScale.ScaleY = newScaleY;

         // Adjust the TranslateTransform to zoom toward the mouse pointer
         PanTransform.X -= (mousePosition.X - PanTransform.X) * (zoomFactor - 1);
         PanTransform.Y -= (mousePosition.Y - PanTransform.Y) * (zoomFactor - 1);
      }

   }
}
