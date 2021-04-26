# Running the Test on the Command Line, Please Use: 'py -W ignore test.py' or 'python -W ignore test.py' 
# Unit Testing of the app.py aliased as 'test.py' for the testing purposes
import testapp as application
import unittest
import tkinter as tk
import os
import subprocess

class IntegerationTestApplication(unittest.TestCase):
    async def start_application1(self):
        self.app.mainloop()
    async def start_application2(self):
        self.app_win.mainloop()
    def setUp(self):
        self.app = application.main()
        with open('fileloc','w') as efile:
            efile.write(os.getcwd()+'/main.cpp')
            efile.close()        
        with open('fileloc','r') as efile:
            loc=efile.read()
            efile.close()
        self.app_win=application.Main(loc)
        self.start_application1()
        self.start_application2()
    def tearDown(self):
        self.app.destroy()
    def test_file_location_save(self):
        self.assertEqual(os.path.isfile('fileloc'),True,'The file does not exist')
        with open('fileloc','r') as efile:
            loc=efile.read()
            efile.close()
        self.assertEqual(os.path.isfile(loc),True,'The Selected File does not exist')
    def test_compiled_source(self):
        self.assertEqual(os.path.isfile('fileloc'),True,'The file does not exist')
        with open('fileloc','r') as efile:
            loc=efile.read()
            efile.close()
        comp=subprocess.run(["g++","-o","main.exe","-fopenmp","main.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input="")
        self.assertEqual(comp.stderr,b'','There is a compilation error in the source file')
    def test_compiled_algo_files(self):
        self.assertEqual(os.path.isfile('SG.cpp'),True,'The algorithm file SG does not exist')
        self.assertEqual(os.path.isfile('SGL.cpp'),True,'The algorithm file SGL does not exist')
        self.assertEqual(os.path.isfile('DG.cpp'),True,'The algorithm file DG does not exist')
        self.assertEqual(os.path.isfile('DGL.cpp'),True,'The algorithm file DGL does not exist')
        self.assertEqual(os.path.isfile('sourceG.cpp'),True,'The algorithm file sourceG does not exist')
        if os.path.isfile('test1.exe') == False:
            subprocess.Popen(["g++","-o","test1.exe","SGL.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        if os.path.isfile('test2.exe') == False:
            subprocess.Popen(["g++","-o","test2.exe","DGL.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        if os.path.isfile('test3.exe') == False:
            subprocess.Popen(["g++","-o","test3.exe","SG.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        if os.path.isfile('test4.exe') == False:
            subprocess.Popen(["g++","-o","test4.exe","DG.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        if os.path.isfile('test.exe') == False:
            subprocess.Popen(["g++","-o","test.exe","sourceG.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        self.assertEqual(os.path.isfile('test1.exe'),True,'The algorithm file test1 does not exist')
        self.assertEqual(os.path.isfile('test2.exe'),True,'The algorithm file test2 does not exist')
        self.assertEqual(os.path.isfile('test3.exe'),True,'The algorithm file test3 does not exist')
        self.assertEqual(os.path.isfile('test4.exe'),True,'The algorithm file test4 does not exist')
        self.assertEqual(os.path.isfile('test.exe'),True,'The algorithm file test does not exist')
    
    def test_file_slice(self):
        input1='39\n1\nz'
        subprocess.run(["test1.exe"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input=bytes(input1,'utf-8'))
        self.assertEqual(os.path.isfile('slice.txt'),True,"The Slice wasn't generated")

    def test_recorder(self):
        self.assertEqual(os.path.isfile('recorder.csv'),True,"The Recorder wasn't generated")
        
    def test_graph_files(self):
        input1='39\n1\nz'
        subprocess.run(["test3.exe"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input=bytes(input1,'utf-8'))
        self.assertEqual(os.path.isfile('AR.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('CD.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('CE.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('I.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('PI.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('PO.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('RL.txt'),True,"The graph files not present")
        self.assertEqual(os.path.isfile('T.txt'),True,"The graph files not present")
        
    def test_file_save(self):
        with open('tempfile','w') as efile:
            efile.write('foo-bar')
            efile.close()
        self.assertEqual(os.path.isfile('tempfile'),True,"The file wasn't created")
        os.remove('tempfile')

    def test_the_destructor(self):
        os.remove('slice.txt')
        os.remove('main.exe')
        os.remove('fileloc')
        os.remove('AR.txt')
        os.remove('CD.txt')
        os.remove('CE.txt')
        os.remove('I.txt')
        os.remove('PI.txt')
        os.remove('PO.txt')
        os.remove('RL.txt')
        os.remove('T.txt')
        self.assertEqual(os.path.isfile('AR.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('CD.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('CE.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('I.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('PI.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('PO.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('RL.txt'),False,"The graph files present")
        self.assertEqual(os.path.isfile('T.txt'),False,"The graph files present")


class UnitTestApplication(unittest.TestCase):
    async def start_application1(self):
        self.app.mainloop()
    async def start_application2(self):
        self.app_win.mainloop()
    def setUp(self):
        self.app = application.main()
        with open('fileloc','w') as efile:
            efile.write(os.getcwd()+'/main.cpp')
            efile.close()
        with open('fileloc','r') as efile:
            loc=efile.read()
            efile.close()
        self.app_win=application.Main(loc)
        self.start_application1()
        self.start_application2()
    def tearDown(self):
        self.app.destroy()

    def test_root_load(self):
        self.assertEqual(self.app.title(),"C++ Program Slicer","the root window didn't load successfully")
    def test_file_select_button(self):
        child=self.app.winfo_children()
        self.assertEqual(child[2].winfo_class(),'Button',"The File Select Button is not loaded successfully")
    def test_system_info_table(self):
        child=self.app.winfo_children()
        self.assertEqual(child[-1].winfo_class(),'Treeview',"The Treeview is not loaded successfully")
    def test_gcc_label(self):
        child=self.app.winfo_children()
        self.assertEqual(child[3].winfo_class(),'Label',"The Treeview is not loaded successfully")

    def test_main_window_load(self):
        self.assertEqual(self.app_win.title(),"C++ Program Slicer","the main window didn't load successfully")
    def test_main_window_menu(self):
        child=self.app_win.winfo_children()
        self.assertEqual(child[3].winfo_class(),'Menu','The MenuBar is not loaded successfully')
    def test_code_preview(self):
        child=self.app_win.winfo_children()[1].winfo_children()[1].winfo_children()
        self.assertEqual(child[0].winfo_class(),'Listbox','The Code Preview is not loaded successfully')
    def test_window_scroll_bar(self):
        child=self.app_win.winfo_children()[1].winfo_children()[1].winfo_children()
        self.assertEqual(child[1].winfo_class(),'Scrollbar','The Vertical Scrollbar is not loaded successfully')
        self.assertEqual(child[2].winfo_class(),'Scrollbar','The Horizontal Scrollbar is not loaded successfully')
    def test_run_button(self):
        child=self.app_win.winfo_children()[1].winfo_children()[2].winfo_children()
        self.assertEqual(child[-1].winfo_class(),'Button','The Run Button is not loaded successfully')
    def test_algo_selection_field(self):
        child=self.app_win.winfo_children()[1].winfo_children()[2].winfo_children()[1].winfo_children()
        self.assertEqual(child[0].winfo_class(),'Label','The Algorithm selection module is not loaded successfully')
        self.assertEqual(child[1].winfo_class(),'Radiobutton','The Radiobutton widget is not loaded successfully')
    def test_variable_field(self):
        child=self.app_win.winfo_children()[1].winfo_children()[2].winfo_children()[2].winfo_children()
        self.assertEqual(child[1].winfo_class(),'Entry','The Variable Entry widget is not loaded successfully')
    def test_stdin_field(self):
        child=self.app_win.winfo_children()[1].winfo_children()[2].winfo_children()[0].winfo_children()[1].winfo_children()
        self.assertEqual(child[0].winfo_class(),'Text','The stdin Text widget is not loaded successfully')
    def test_slice_tab(self):
        child=self.app_win.winfo_children()[2].winfo_children()[0].winfo_children()[0].winfo_children()
        self.assertEqual(child[0].winfo_class(),'Text','The Slice tab is not loaded successfully')
        self.assertEqual(child[1].winfo_class(),'Scrollbar','The Vertical Scrollbar is not loaded successfully')
        self.assertEqual(child[2].winfo_class(),'Scrollbar','The Horizontal Scrollbar is not loaded successfully')
    def test_stdout_tab(self):
        child=self.app_win.winfo_children()[2].winfo_children()[0].winfo_children()[1].winfo_children()
        self.assertEqual(child[0].winfo_class(),'Text','The stdout tab is not loaded successfully')
        self.assertEqual(child[1].winfo_class(),'Scrollbar','The Vertical Scrollbar is not loaded successfully')
        self.assertEqual(child[2].winfo_class(),'Scrollbar','The Horizontal Scrollbar is not loaded successfully')

    def test_constructor(self):
        application.Constructor(tk.Label())
        self.assertEqual(os.path.isfile('test.exe'),True,'The Constructor did not function successfully')

    def test_input_view(self):
        val=application.InputView(0,tk.IntVar(),tk.Text())
        self.assertEqual(val,0,'The InputView did not function successfully')

if __name__=="__main__":
    print("Starting the Integeration Tests...")
    itsuite=unittest.TestSuite()
    itsuite.addTest(unittest.makeSuite(IntegerationTestApplication))
    itrunner=unittest.TextTestRunner(verbosity=2)
    itrunner.run(itsuite)
    print("Starting the Unit Tests...")
    utsuite=unittest.TestSuite()
    utsuite.addTest(unittest.makeSuite(UnitTestApplication))
    utrunner=unittest.TextTestRunner(verbosity=2)
    utrunner.run(utsuite)
    # unittest.main(verbosity=2)