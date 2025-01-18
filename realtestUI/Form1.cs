using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Pipes;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace realtestUI
{

    public partial class Form1 : Form
    {
        [DllImport("module.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Run();

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void button1_Click(object sender, EventArgs e)
        {
            new Thread(() =>
            {
                Run();
            }).Start();
        }
        

        private void button2_Click(object sender, EventArgs e)
        {
            
        }
    }
}
