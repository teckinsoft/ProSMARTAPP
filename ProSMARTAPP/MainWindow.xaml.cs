using IGESWrapper;
using System;
using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Imaging;

namespace ProSMARTAPP {
   public partial class MainWindow : Window {
      IGESHandlerWrapper igesHandler;

      public MainWindow () {
         InitializeComponent ();
      }

      void LoadPart (string filename, int order) {
         try {
            // Initialize and use IGESHandlerWrapper
            if (igesHandler == null)
               igesHandler = new IGESHandlerWrapper ();
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

      private async void OnPart2Click (object sender, RoutedEventArgs e) {
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

      private void DisplayInputsImage () {
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
      private void DisplayOutputImage () {
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

      private void OnMouseWheel (object sender, MouseWheelEventArgs e) {
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
      private void OnUnionClick (object sender, RoutedEventArgs e) {
         try {
            // Perform union operation on the two parts
            igesHandler.UnionShapes ();

            // Generate the union image and display it
            var unionImageData = igesHandler.DumpFusedShape (800, 600);

            DisplayOutputImage ();
            // Display the generated PNG image
            using (var ms = new MemoryStream (unionImageData)) {
               var bitmap = new BitmapImage ();
               bitmap.BeginInit ();
               bitmap.CacheOption = BitmapCacheOption.OnLoad;
               bitmap.StreamSource = ms;
               bitmap.EndInit ();
               ImageControl.Source = bitmap;
            }

            // Open a File Save dialog to save the resulting IGES file
            var saveFileDialog = new Microsoft.Win32.SaveFileDialog {
               Title = "Save Unioned IGES File",
               DefaultExt = ".igs",
               Filter = "IGES Files (*.igs)|*.igs|IGES Files (*.iges)|*.iges",
               InitialDirectory = @"W:\FChassis\Sample",
               FileName = "UnionResult.igs"
            };

            if (saveFileDialog.ShowDialog () == true) {
               // Save the unioned shape to the selected file
               igesHandler.SaveIGES (saveFileDialog.FileName, 2); // Assuming order 0 is for the union result
               MessageBox.Show ($"Unioned IGES file saved successfully to {saveFileDialog.FileName}");
            }
         } catch (Exception ex) {
            MessageBox.Show ($"Error during union operation: {ex.Message}");
         }
      }
   }
}
