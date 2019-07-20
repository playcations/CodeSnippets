using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

namespace MLDisplayFind
{
    public partial class Form1 : Form
    {
        StreamReader sr;

        //Initialize 2D Jagged List
        List<List<double>> rows;

        public Form1()
        {
            InitializeComponent();
        }

        private void textBox1_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsControl(e.KeyChar) && !char.IsDigit(e.KeyChar) && (e.KeyChar != '.'))
            {
                e.Handled = true;
            }

            if ((e.KeyChar == '.') && ((sender as TextBox).Text.IndexOf('.') > -1))
            {
                e.Handled = true;
            }
        }

        private void ChooseFile_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                sr = new StreamReader(openFileDialog1.FileName);
                rows = new List<List<double>>();
                BuildRows(rows);
                MessageBox.Show("File selected");

            }
            else
            {
                MessageBox.Show("No File selected");
            }
        }


        private void FindResults_Click(object sender, EventArgs e)
        {
            if (sr != null && textBox1.Text.Length > 0)
            {
                int numCols = rows[0].Count;
                int numRows = rows.Count;
                
                double minNitValue = double.Parse(textBox1.Text);
                minNitValue = minNitValue / 204.25D;

                double threshWidth = double.Parse(textBox4.Text);
                double threshHeight = double.Parse(textBox5.Text);

                Boolean foundAllRect = false;
                int rectNum = 1;

                //Used for finding relative FOV
                double headsetHFOV = double.Parse(textBox2.Text);
                double headsetVFOV = double.Parse(textBox3.Text);

                //First, convert to 2D binary-valued List
                List<List<int>> binArray = BinaryConversion(rows, minNitValue);

                while (foundAllRect == false)
                {
                    //Find largest rect 
                    Tuple<int, int, int, int, int> largestRec = maxRectangle(numRows, numCols, binArray);


                    /*
                     *Use For Debugging:
                     * 
                    for (int i = 0; i < binArray.Count; i++)
                    {
                        string newLine = "";
                        for (int k = 0; k < binArray[0].Count; k++)
                        {
                            newLine += binArray[i][k];
                        }
                        richTextBox1.Text += "\n" + newLine;
                    }
                    */

                    //(x,y) are given assuming (0,0) is TOP left corner of CSV file; and y increases downwards
                    //flip y coord so that (0,0) is BOTTOM left corner of CSV file
                    int y = numRows -(largestRec.Item3 + 1);
                    int x = largestRec.Item2;
                    int center_x = largestRec.Item2 + (largestRec.Item5 / 2);
                    int center_y = y - (largestRec.Item4 / 2);
                    int area = largestRec.Item1;
                    int height = largestRec.Item4;
                    int width = largestRec.Item5;
                    double horizFOVmax = headsetHFOV * ((double)largestRec.Item5 / (double)numCols);
                    double vertFOVmax = headsetVFOV * ((double)largestRec.Item4 / (double)numRows);

                    //check if below thresholds
                    if ((vertFOVmax < threshHeight && horizFOVmax < threshWidth))
                    {
                        foundAllRect = true;
                        richTextBox1.Text += "\n\nNo more Rectangles exceeding Thresholds";
                        break;

                    }
                   else if ( (vertFOVmax < threshHeight || horizFOVmax < threshWidth) )
                    {
                        //foundAllRect = true;
                        richTextBox1.Text += "\n\nNo more Rectangles exceeding Thresholds";
                        BlackOut(binArray, x, largestRec.Item3, height, width);
                        continue;
                        
                    }

                    //Must be above thresholds, print information
                    richTextBox1.Text +=
                    "\n\nRectangle " + rectNum + ":" +
                    "\n\nArea is = " + area +
                    "\nHeight is  = " + height +
                    "\nWidth is = " + width +
                    "\n(x, y) of top left corner is = " + '(' + x + ", " + y + ')' +
                    "\n(x,y) coordinate of center pixel is =  " + "(" + center_x + "," + " " + center_y + ")" +
                    "\nHorizFOV with all pixels above or equal to threshold is = " + horizFOVmax + " degrees" +
                    "\nVertFOV with all pixels above or equal to threshold is = " + vertFOVmax + " degrees";

                    //moving onto next rectangle
                    rectNum++;
                    richTextBox1.Text += "\nFinding Next Rectangle...";

                    //Black out, run again
                    //Give non-flipped y, makes indexing easier in BlackOut
                    BlackOut(binArray, x, largestRec.Item3, height, width);
                }
                    
            }

            else
            {
                string message = "";
                if(sr == null)
                {
                    message += "No File Selected!\n";
                }
                if(textBox1.Text.Length == 0)
                {
                    message += "Nits not set!\n";
                }
                if(textBox2.Text.Length == 0)
                {
                    message += "Headset's Horizontal FOV not set!\n";
                }
                if (textBox3.Text.Length == 0)
                {
                    message += "Headset's Vertical FOV not set!\n";
                }
                MessageBox.Show(message);
            }
        }

        /// <summary>
        /// Sets rectangle in A to 0s
        /// </summary>
        /// <param name="A"> binary-valued 2D jagged array </param>
        /// <param name="x"> top left x coord of rectangle </param>
        /// <param name="y"> top left y coord of rectangle; uses row indexing (top is 0, increases going downwards) </param>
        /// <param name="height"></param>
        /// <param name="width"></param>
        private static void BlackOut(List<List<int>> A, int x, int y, int height, int width)
        {
            //(x,y) -> A[y][x]

            for (int i = y; i < y + height; i++)
            {
                for (int j = x; j < x+width; j++)
                {
                    A[i][j] = 0;
                }
            }
        }

        /// <summary>
        /// Converts 2D Jagged List of luminance values 
        /// into 2D Jagged List of binary values, and returns this binary 2D List.
        /// 1 -> entry is above threshold
        /// 0 -> entry is below threshold
        /// </summary>
        /// <param name="rows">2D Jagged List of luminance values</param>
        /// <param name="minNitValue">threshold value</param>
        private List<List<int>> BinaryConversion(List<List<double>> rows, double minNitValue)
        {
            List<List<int>> binArray  = new List<List<int>>();

            int numRows = rows.Count;
            int numColumns = rows[0].Count;

            for (int i = 0; i < numRows; i++)
            {
                List<int> currRow = new List<int>();

                for (int k = 0; k < numColumns; k++)
                {

                    if (rows[i][k] >= minNitValue)
                    {
                        currRow.Add(1);
                    }
                    else
                    {
                        currRow.Add(0);
                    }
                }
                binArray.Add(currRow);
            }
            return binArray;
        }

        /// <summary>
        /// Returns largest rectangle under Histogram's (area, starting index, height, width) as a Tuple.
        /// Used as a subroutine for finding largest rectangle of 1s in 2D Array.
        /// Modified version of: https://www.geeksforgeeks.org/largest-rectangle-under-histogram/
        /// </summary>
        /// <param name="hist">List which represents Histogram</param>
        /// <param name="n">Number of bars in the Histogram</param>
        private static Tuple<int, int, int, int> maxHist(List<int> hist,
                             int n)
        {
            // Create an empty stack. The stack  
            // holds indexes of hist[] array  
            // The bars stored in stack are always  
            // in increasing order of their heights.  
            Stack<int> s = new Stack<int>();

            int max_area = 0; // Initialize max area 
            int tp; // To store top of stack 
            int area_with_top; // To store area with top  
                               // bar as the smallest bar 
            int max_rect_start_ind = 0;
            int max_rect_height = 0;
            int max_rect_width = 0;

            // Run through all bars of 
            // given histogram  
            int i = 0;
            while (i < n)
            {
                // If this bar is higher than the  
                // bar on top stack, push it to stack 
                // 
                // TLDR: if it's in the stack, you still don't know final max area yet using that
                // value in the stack, you need to keep going (increasing the width of potential rectangle; hence i++)
                // until you encounter a smaller height, at which point you use hist[stack top] (and the increasing width
                // you've tallyed up so far) to calculate the max area using hist[stack top]
                if (s.Count == 0 || hist[s.Peek()] <= hist[i])
                {
                    s.Push(i++);
                }

                // If this bar is lower than top of stack, 
                // then calculate area of rectangle with  
                // stack top as the smallest (or minimum   
                // height) bar. 'i' is 'right index' (pictorally, index after rectangle ends) for the top;
                // element before top in stack is 'left index' (pictorally, index before where the rectangle starts)
                // 
                // We cannot however say anything about location of stack top (w/ respect to rectangle)
                // other than it's the height of the rectangle 
                else
                {
                    tp = s.Peek(); // store the top index 
                    s.Pop(); // pop the top 

                    // Calculate the area with hist[tp] 
                    // stack as smallest bar 
                    area_with_top = hist[tp] *
                                   (s.Count == 0 ? i : i - s.Peek() - 1);

                    // update max area, if needed
                    // Also note the index of where the rectangle starts
                    if (max_area < area_with_top)
                    {
                        max_area = area_with_top;

                        //Note: we already popped top index at this point,
                        //so stack holds element before top, which is index b4 where rectangle starts; hence the +1

                        //Also note that if the stack is empty, means that there are no smaller rectangles
                        //earlier in the histogram; hence, your rectangle starts from 0 and spans all the way to i-1
                        max_rect_start_ind = (s.Count == 0 ? 0 : s.Peek() + 1);
                        max_rect_height = hist[tp];
                        max_rect_width = (s.Count == 0 ? i : i - s.Peek() - 1);

                    }

                    //Note: DIDN'T update i in this entire else conditional.
                    //Why?
                    //The point of i is to tally up "potential" widths for rectangles in the stack.
                    //if hist[i] < hist[stack top], we can't continue increasing the "potential" width 
                    //for the hist[stack top] rectangle b/c then we would be creating a rectangle 
                    //that's missing an upper right chunk of area.
                }
            }

            // Now pop the remaining bars from  
            // stack and calculate area with every  
            // popped bar as the smallest bar  
            while (s.Count > 0)
            {
                tp = s.Peek();
                s.Pop();
                area_with_top = hist[tp] *
                               (s.Count == 0 ? i : i - s.Peek() - 1);

                if (max_area < area_with_top)
                {
                    max_area = area_with_top;
                    max_rect_start_ind = (s.Count == 0 ? 0 : s.Peek() + 1);
                    max_rect_height = hist[tp];
                    max_rect_width = (s.Count == 0 ? i : i - s.Peek() - 1);
                }
            }
            return Tuple.Create(max_area, max_rect_start_ind, max_rect_height, max_rect_width);
        }

        /// <summary>
        /// Returns largest rectangle's (area, x, y, height, width) as a Tuple.
        /// Modified version of: https://www.geeksforgeeks.org/maximum-size-rectangle-binary-sub-matrix-1s/
        /// </summary>
        /// <param name="R">Number of rows</param>
        /// <param name="C">Number of columns</param>
        /// <param name="A">Jagged "2D" array</param>
        private static Tuple<int, int, int, int, int> maxRectangle(int R, int C,
                                       List<List<int>> A)
        {
            // Calculate area for first row  
            // and initialize it as result  
            Tuple<int, int, int, int> max_area_startIndex_height_width = maxHist(A[0], A[0].Count);

            //Initialize maxArea, (x,y) coord, height, and width
            int maxArea = max_area_startIndex_height_width.Item1;
            int max_x_coord = max_area_startIndex_height_width.Item2;
            int max_height = max_area_startIndex_height_width.Item3;
            int max_width = max_area_startIndex_height_width.Item4;
            int max_y_coord = 0;

            //Don't want to modify A because need to run recursive function on A later
            //Create a copy
            var copyA = new List<List<int>>();
            for (int i = 0; i < R; i++)
            {
                var rowCopy = new List<int>();

                for (int j = 0; j < C; j++)
                {
                    rowCopy.Add(A[i][j]);
                }

                copyA.Add(rowCopy);
            }

            // iterate over row to find 
            // maximum rectangular area  
            // considering each row as histogram  
            for (int i = 1; i < R; i++)
            {
                for (int j = 0; j < C; j++)
                {

                    // if A[i][j] is 1 then 
                    // add A[i -1][j]  
                    if (copyA[i][j] == 1)
                    {
                        copyA[i][j] += copyA[i - 1][j];
                    }
                }
                // Update result if area with current  
                // row (as last row of rectangle) is more
                Tuple<int, int, int, int> potential_max = maxHist(copyA[i], copyA[i].Count);
                int potential_maxArea = potential_max.Item1;

                if (potential_maxArea > maxArea)
                {
                    maxArea = potential_maxArea;
                    max_x_coord = potential_max.Item2;
                    max_y_coord = i - potential_max.Item3 + 1; //+1 because (i - height) gives y index b4 top left corner
                    max_height = potential_max.Item3;
                    max_width = potential_max.Item4;

                }
            }

            return Tuple.Create(maxArea, max_x_coord, max_y_coord, max_height, max_width);
        }

        /// <summary>
        /// Builds 2D Jagged List from CSV file
        /// </summary>
        /// <param name="rows">Pointer for 2D Jagged List</param>
        private void BuildRows(List<List<double>> rows) 
        {
            string currentRowline;
            while (!sr.EndOfStream)
            {
                currentRowline = sr.ReadLine();
                string[] splitrow = currentRowline.Split(',');
                List<double> currentRow = new List<double>();

                for (int i = 0; i < splitrow.Length; i++)
                {

                    double currentNum = Convert.ToDouble(splitrow[i]);
                    currentRow.Add(currentNum);
                }
                rows.Add(currentRow);
            }
        }
    }
}
