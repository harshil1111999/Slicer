import tkinter as tk
from tkinter import filedialog
from tkinter import messagebox
from tkinter import ttk
import time
import os
import shutil
import subprocess
import platform
import psutil
import networkx as nx
import graphviz
import pydot
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

def Constructor(status):
    status.config(text="Running functions...")
    if os.path.isfile('test1.exe') == False:
        comp=subprocess.Popen(["g++","-o","test1.exe","SGL.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        comp.wait()
    if os.path.isfile('test2.exe') == False:
        comp=subprocess.Popen(["g++","-o","test2.exe","DGL.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        comp.wait()
    if os.path.isfile('test3.exe') == False:
        comp=subprocess.Popen(["g++","-o","test3.exe","SG.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        comp.wait()
    if os.path.isfile('test4.exe') == False:
        comp=subprocess.Popen(["g++","-o","test4.exe","DG.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        comp.wait()
    # status.config(text="Ready...")

def Destructor(window):
    if messagebox.askokcancel("Quit", "Do you want to quit?"):
        if os.path.isfile(os.getcwd()+"/main.exe"):
            os.remove(os.getcwd()+"/main.exe")
        if os.path.isfile("test.exe"):
            os.remove("test.exe")
        if os.path.isfile("test1.exe"):
            os.remove("test1.exe")
        if os.path.isfile("fileloc"):
            os.remove("fileloc")
        if os.path.isfile("test2.exe"):
            os.remove("test2.exe")
        if os.path.isfile("test3.exe"):
            os.remove("test3.exe")
        if os.path.isfile("test4.exe"):
            os.remove("test4.exe")
        if os.path.isfile("CD.txt"):
            os.remove("CD.txt")
        if os.path.isfile("AR.txt"):
            os.remove("AR.txt")
        if os.path.isfile("I.txt"):
            os.remove("I.txt")
        if os.path.isfile("T.txt"):
            os.remove("T.txt")
        if os.path.isfile("PI.txt"):
            os.remove("PI.txt")
        if os.path.isfile("PO.txt"):
            os.remove("PO.txt")
        if os.path.isfile("CE.txt"):
            os.remove("CE.txt")
        if os.path.isfile("RL.txt"):
            os.remove("RL.txt")
        if os.path.isfile("temp_source.cpp"):
            os.remove("temp_source.cpp")
        window.destroy()
    exit()
def savefile():
    fileloc=filedialog.asksaveasfile(mode='w', defaultextension=".jpeg", filetypes=(("JPEG file", "*.jpeg"),("PNG file", "*.png*") ))
    if fileloc is None:
        return
    else:
        print((fileloc.name))
        plt.savefig(str(fileloc.name))
def GraphMaker(windowtitle):
    G=nx.DiGraph()
    with open('CD.txt') as data:
        CDnodes=[]
        CDedges=[]
        CDlines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            #line=list(map(lambda txt: txt.lstrip(),line))
            CDlines.append(line)
        data.close()
        for line in CDlines:
            CDnodes.append(line[0])
            for i in range(1,len(line)):
                CDedges.append((line[i],line[0]))
        G.add_nodes_from(CDnodes)
        G.add_edges_from(CDedges,color='red',length=0.1)
    with open('AR.txt') as data:
        ARedges=[]
        ARlines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            ARlines.append(line)
        data.close()
        for line in ARlines:
            for i in range(1,len(line)):
                ARedges.append((line[i],line[0]))
        G.add_edges_from(ARedges,color='blue',length=0.2)
    with open('I.txt') as data:
        Iedges=[]
        Ilines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            Ilines.append(line)
        data.close()
        for line in Ilines:
            for i in range(1,len(line)):
                Iedges.append((line[i],line[0]))
        G.add_edges_from(Iedges,color='yellow',length=0.3)
    with open('T.txt') as data:
        Tedges=[]
        Tlines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            Tlines.append(line)
        data.close()
        for line in Tlines:
            for i in range(1,len(line)):
                Tedges.append((line[i],line[0]))
        G.add_edges_from(Tedges,color='green',length=0.4)
    with open('PI.txt') as data:
        PIedges=[]
        PIlines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            PIlines.append(line)
        data.close()
        for line in PIlines:
            for i in range(1,len(line)):
                PIedges.append((line[i],line[0]))
        print(PIedges)
        G.add_edges_from(PIedges,color='cyan',length=0.5)
    with open('PO.txt') as data:
        POedges=[]
        POlines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            POlines.append(line)
        data.close()
        for line in POlines:
            for i in range(1,len(line)):
                POedges.append((line[i],line[0]))
        print(POedges)
        G.add_edges_from(POedges,color='magenta',length=0.6)
    with open('CE.txt') as data:
        CEedges=[]
        CElines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            CElines.append(line)
        data.close()
        for line in CElines:
            for i in range(1,len(line)):
                CEedges.append((line[i],line[0]))
        print(CEedges)
        G.add_edges_from(CEedges,color='violet',length=0.7)
    with open('RL.txt') as data:
        RLedges=[]
        RLlines=[]
        for line in data.readlines():
            line=line[:-1]
            line=line.split('|')
            RLlines.append(line)
        data.close()
        for line in RLlines:
            for i in range(1,len(line)):
                RLedges.append((line[i],line[0]))
        print(RLedges)
        G.add_edges_from(RLedges,color='purple',length=0.8)
    if os.path.isfile("CD.txt"):
        os.remove("CD.txt")
    if os.path.isfile("AR.txt"):
        os.remove("AR.txt")
    if os.path.isfile("I.txt"):
        os.remove("I.txt")
    if os.path.isfile("T.txt"):
        os.remove("T.txt")
    if os.path.isfile("PI.txt"):
        os.remove("PI.txt")
    if os.path.isfile("PO.txt"):
        os.remove("PO.txt")
    if os.path.isfile("CE.txt"):
        os.remove("CE.txt")
    if os.path.isfile("RL.txt"):
        os.remove("RL.txt")
    labels={}
    for node in G.nodes():
        labels[node]=str(node).split(':')[0]
    # print(labels)
    edgecolor=nx.get_edge_attributes(G,'color').values()
    print(set(edgecolor))
    CDpatch=mpatches.Patch(color='red',label="Control/Data Dependent Edges")
    ARpatch=mpatches.Patch(color='blue',label="Affect Return Edges")
    Ipatch=mpatches.Patch(color='yellow',label="Interference Edges")
    Tpatch=mpatches.Patch(color='green',label="Transitive Edges")
    PIpatch=mpatches.Patch(color='cyan',label="Parameter In Edges")
    POpatch=mpatches.Patch(color='magenta',label="Parameter Out Edges")
    CEpatch=mpatches.Patch(color='violet',label="Calling Edges")
    RLpatch=mpatches.Patch(color='purple',label="Return Link Edges")
    fig=plt.figure(figsize=(20,20))
    pos = nx.nx_pydot.graphviz_layout(G,prog='neato')
    nx.draw_networkx(G,pos,with_labels=False,edge_color=edgecolor,alpha=0.7,node_size=400,node_color="skyblue", node_shape="s",connectionstyle="arc3,rad=0.15")
    nx.draw_networkx_labels(G,pos,labels,font_size=8)
    fig.legend(handles=[CDpatch,ARpatch,Ipatch,Tpatch,PIpatch,POpatch,CEpatch,RLpatch])
    toplevel=tk.Toplevel()
    toplevel.state('zoomed')
    toplevel.title(windowtitle)
    tk.Button(toplevel,text="Save Image...",command=lambda: savefile()).pack(side=tk.TOP, fill=tk.X, expand=True)
    dataPlot = FigureCanvasTkAgg(fig, master=toplevel)
    dataPlot.draw()
    dataPlot.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
    dataPlot._tkcanvas.pack(side="top", fill="both", expand=True)
    tk.Button(toplevel,text="Save",command=lambda: savefile()).pack() 
    # toplevel.mainloop()
    # dataPlot.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

def TargetFileRunner(filelocation,status,codepreview,nongraph,stdoutoutput,variablenames,output,InputEntry,flag):
    start_time=time.perf_counter()
    comp=subprocess.run(["g++","-o","main.exe","-fopenmp",filelocation],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input="")
    if comp.stderr!=b'':
        messagebox.showerror("Compilation Errors: Please correct and load the file again",comp.stderr)
    else:
        if codepreview.curselection():
            line=codepreview.get(codepreview.curselection())
            linenumber=line.split(' ',1)[0]
            # print(line.find(variablenames.get()))
            # print(variablenames.get(),type(variablenames.get()),len(variablenames.get()))
            if len(variablenames.get())!=0 and line.find(variablenames.get())!=-1:
                Constructor(status)
                status.config(text="Running...")
                if flag.get()%2==0:
                    stdin=InputEntry.get('1.0','end-1c')
                    print(stdin)
                    input1=str(linenumber)+'\n'+'1\n'+variablenames.get()+'\n'+stdin
                else:
                    input1=str(linenumber)+'\n'+'1\n'+variablenames.get()+'\n'
                input1=bytes(input1,'utf-8')
                espn=subprocess.run(["test"+str(flag.get())+".exe"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input=input1)
                stdout=espn.stdout.decode('UTF-8')
                stdoutoutput.config(state='normal')
                stdoutoutput.delete('1.0',tk.END)
                if flag.get()%2==0:    
                    stdoutoutput.insert(tk.END,stdout)
                else:
                    stdoutoutput.insert(tk.END,"No stdout generated for Static algorithms")
                stdoutoutput.config(state='disabled')
                # print(stdout)
                slicetext=open('slice.txt','r').read()
                os.remove('slice.txt')
                output.config(state='normal')
                output.delete('1.0',tk.END)
                output.insert(tk.END,slicetext)
                output.config(state='disabled')
                status.config(text="Done...")
                algo=["Static Graphless","Dynamic Graphless","Static Graph","Dynamic Graph"]
                if flag.get()>2 and flag.get()<5:
                    GraphMaker(algo[flag.get()-1]+"Method SDG")
                # print(espn.stdout)
                exe_time=time.perf_counter()-start_time
                slice_line_count=int(output.index('end-1c').split('.')[0])
                code_line_count=len(codepreview.get(0,"end"))
                algo=["Static Graphless","Dynamic Graphless","Static Graph","Dynamic Graph"]
                # print(algo[flag.get()-1],exe_time,code_line_count,slice_line_count)
                with open('recorder.csv','a') as rec:
                    rec.write(str(algo[flag.get()-1])+','+str(exe_time)+','+str(code_line_count)+','+str(slice_line_count)+'\n')
                    rec.close()
            else:
                messagebox.showwarning("Enter the Variable Name","Please Select a valid variable name from the line")    
        else:
            messagebox.showwarning("Select a line","Please select a line from the Code Preview Whose slice needs to be calculated.")
def InputView(flag,flag1,InputEntry):
    flag1.set(flag)
    # print(flag1.get())
    if flag%2==0: 
        InputEntry.config(state='normal')
    else:
        InputEntry.delete('1.0',tk.END)
        InputEntry.config(state='disabled')
    return flag

def FileSelector(root): #Selects a file and copies to the current working directory irrespective of the OS
    root.withdraw()
    filelocation=filedialog.askopenfilename(title="Select a C++ file",filetypes=[("C++ file",'*.cpp')])
    if filelocation is None or filelocation=='':
        exit()
    with open("fileloc",'w') as fileloc:
        fileloc.write(filelocation)
        fileloc.close()
    window=Main(filelocation)
    window.mainloop()

def Close(root):
    root.destroy()
    exit()
def New(window):
    FileSelector(window)
def SaveEditFile(filelocation,editfile,editwindow,window):
    text=editfile.get('1.0','end-1c')
    with open(filelocation,'w') as savefile:
        savefile.write(text)
        savefile.close()
    editwindow.destroy()
    window.destroy()
    Main(filelocation)

def Edit(filelocation,window):
    with open(filelocation,'r') as file:
        text=file.read()
        file.close()
    editwindow=tk.Toplevel()
    editwindow.title("Edit...")
    editwindow.focus_force()
    editfile=tk.Text(editwindow,width=75,height=40,font=("Input",11),fg='#FFFFFF',bg='#093145')
    scrollv=tk.Scrollbar(editwindow,orient="vertical",command = editfile.yview)
    scrollv.pack(side=tk.RIGHT,fill=tk.Y)
    editfile.config(yscrollcommand=scrollv.set)
    editfile.insert(tk.END,text)
    editfile.pack(pady=10)
    savebutton=tk.Button(editwindow,text="Save",command=lambda:SaveEditFile(filelocation,editfile,editwindow,window))
    savebutton.pack(padx=5,pady=5)
    editwindow.mainloop()
def SliceCopy(output):
    output.config(state='normal')
    slicetext=output.get('1.0','end-1c')
    output.config(state='disabled')
    tempwin=tk.Tk()
    tempwin.withdraw()
    tempwin.clipboard_clear()
    tempwin.clipboard_append(slicetext)
    tempwin.update() 
    tempwin.destroy()
def SourceGraph():
    if os.path.isfile('test.exe') == False:
        comp=subprocess.Popen(["g++","-o","test.exe","sourceG.cpp"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        comp.wait()
    subprocess.run(["test.exe"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    GraphMaker("Source File System Dependence Graph")
    
def Main(filelocation):
    window=tk.Tk()
    window.state('zoomed')
    window.title("C++ Program Slicer")

    status=tk.Label(window,text="Ready...",bd=1,relief=tk.SUNKEN,anchor=tk.E)
    status.pack(side=tk.BOTTOM,fill=tk.X)
    codeframe=tk.Frame(window)
    codeframe.pack(side=tk.LEFT)
    tk.Label(codeframe,text="File Preview: ",font=("Times",12),justify=tk.LEFT, anchor="n").pack()
    codebox=tk.Frame(codeframe)
    codebox.pack()
    visualization=tk.Frame(window)
    visualization.pack(side=tk.RIGHT)

    tab=ttk.Notebook(visualization)
    nongraph=tk.Frame(tab,bg='#36454F')
    stdouttab=tk.Frame(tab,bg='#35465F')
    tab.add(nongraph,text="Slice")
    tab.add(stdouttab,text="stdout")
    output=tk.Text(nongraph,width=75,height=100,font=("Input",14),fg='#FFFFFF',bg='#36454F',state='disabled')
    stdoutoutput=tk.Text(stdouttab,width=75,height=100,font=("Input",14),fg='#FFFFFF',bg='#36454F',state='disabled')        
    gscrollv=tk.Scrollbar(nongraph,orient="vertical",command = output.yview)
    gscrollv.pack(side=tk.RIGHT,fill=tk.Y)
    output.config(yscrollcommand=gscrollv.set)
    gscrollh=tk.Scrollbar(nongraph,orient="horizontal",command = output.xview)
    gscrollh.pack(side=tk.BOTTOM,fill=tk.X)
    output.config(xscrollcommand=gscrollh.set)
    output.pack()

    ggscrollv=tk.Scrollbar(stdouttab,orient="vertical",command = stdoutoutput.yview)
    ggscrollv.pack(side=tk.RIGHT,fill=tk.Y)
    stdoutoutput.config(yscrollcommand=ggscrollv.set)
    ggscrollh=tk.Scrollbar(stdouttab,orient="horizontal",command = stdoutoutput.xview)
    ggscrollh.pack(side=tk.BOTTOM,fill=tk.X)
    stdoutoutput.config(xscrollcommand=ggscrollh.set)
    stdoutoutput.pack()
    tab.pack()

    codepreview=tk.Listbox(codebox,font=("Roboto",14),width=75,height=20)
    with open(filelocation,"r") as code:
        lines=code.readlines()
        i=0
        for line in lines:
            codepreview.insert(tk.END,str(i+1)+" "+str(line))
            i=i+1
        code.close()
    FPscrollv=tk.Scrollbar(codebox,orient="vertical",command = codepreview.yview)
    FPscrollv.pack(side=tk.RIGHT,fill=tk.Y)
    codepreview.config(yscrollcommand=FPscrollv.set)
    FPscrollh=tk.Scrollbar(codebox,orient="horizontal",command = codepreview.xview)
    codepreview.pack()
    FPscrollh.pack(side=tk.BOTTOM,fill=tk.X)
    codepreview.config(xscrollcommand=FPscrollh.set)

    SelectAlgo=tk.Frame(codeframe,bg='#DCDCDC')
    InputFrame=tk.Frame(SelectAlgo,bg='#DCDCDC')
    RadioFrame=tk.Frame(SelectAlgo)
    InputLabel=tk.Label(InputFrame,text="Input(stdin): ",bg='#DCDCDC')
    InputEntryFrame=tk.Frame(InputFrame)
    InputEntry=tk.Text(InputEntryFrame,width=15,height=3)
    IEscrollv=tk.Scrollbar(InputEntryFrame,orient="vertical",command = InputEntry.yview)
    IEscrollv.pack(side=tk.RIGHT,fill=tk.Y)
    InputEntry.config(yscrollcommand=IEscrollv.set)

    flag=tk.IntVar()
    tk.Label(RadioFrame,text="Algorithm: ",bg='#DCDCDC').pack(side="left")
    radio1=tk.Radiobutton(RadioFrame,text="Static Graphless",variable=flag,value=1,command=lambda: InputView(1,flag,InputEntry),bg='#DCDCDC')
    radio1.pack(side="left")
    radio2=tk.Radiobutton(RadioFrame,text="Dynamic Graphless",variable=flag,value=2,command=lambda: InputView(2,flag,InputEntry),bg='#DCDCDC')
    radio2.pack(side="left")
    radio3=tk.Radiobutton(RadioFrame,text="Static Graph",variable=flag,value=3,command=lambda: InputView(3,flag,InputEntry),bg='#DCDCDC')
    radio3.pack(side="left")
    radio4=tk.Radiobutton(RadioFrame,text="Dynamic Graph",variable=flag,value=4,command=lambda: InputView(4,flag,InputEntry),bg='#DCDCDC')
    radio4.pack(side="left")
    RadioFrame.pack(padx=5,pady=5)
    SelectAlgo.pack()
    VariableFrame=tk.Frame(SelectAlgo)
    variablenameLabel=tk.Label(VariableFrame,text="Variable Names: ",bg='#DCDCDC')
    variablenames=tk.Entry(VariableFrame)
    variablenameLabel.pack(side="left")
    variablenames.pack(side="left")
    VariableFrame.pack(padx=5,pady=5)
    InputFrame.pack(padx=5,pady=5)
    radio1.invoke()
    InputLabel.pack(side="left")
    InputEntryFrame.pack()
    InputEntry.pack(side="left")
    InputFrame.pack()
    runbutton=tk.Button(SelectAlgo,text="Run",command=lambda: TargetFileRunner(filelocation,status,codepreview,nongraph,stdoutoutput,variablenames,output,InputEntry,flag))
    runbutton.pack()

    Menubar=tk.Menu(window)
    Menubar.add_command(label="New",command=lambda: New(window))
    Menubar.add_command(label="Edit",command=lambda: Edit(filelocation,window))
    Menubar.add_command(label="Quit(Alt+F4)",command=lambda: Destructor(window))
    Menubar.add_command(label="Copy Slice", command=lambda: SliceCopy(output))
    Menubar.add_command(label="Source File Graph", command=lambda: SourceGraph())
    window.config(menu=Menubar)

    window.protocol('WM_DELETE_WINDOW',lambda: Destructor(window))
    return window
    # window.mainloop()
    
def main():
    root=tk.Tk()
    tk.Label(root,text="Welcome to the C++ OpenMP Program Slicer",font=("Arial 14 bold")).pack(padx=10,pady=10)
    tk.Label(root,text="Requirement: ",font=("Arial 10 bold")).pack()
    Cppcompiler=subprocess.run(["g++","-v"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    SelectFileButton=tk.Button(root,text="Select File...",command=lambda: FileSelector(root))
    if Cppcompiler.stderr!=b'':
        tk.Label(root,text="• GCC C++ Compiler: Found",bg='green',font=("Arial 8 bold")).pack(padx=2,pady=2)
    else:
        tk.Label(root,text="• GCC C++ Compiler: Not Found",bg='red',font=("Arial 8 bold")).pack(padx=2,pady=2)
        SelectFileButton["state"]="disabled"
    tk.Label(root,text="System Information: ",font=("Arial 10 bold")).pack()
    style = ttk.Style()
    style.configure("customstyle.Treeview", font=('Calibri', 10)) 
    style.configure("customstyle.Treeview.Heading", font=('Calibri', 12,'bold')) 
    Table=ttk.Treeview(root,style="customstyle.Treeview")

    Table['columns']=('SP','V')
    Table.column('#0', width=0, stretch=tk.NO)
    Table.column('SP', anchor=tk.CENTER, width=200)
    Table.column('V', anchor=tk.CENTER, width=400)

    Table.heading('#0', text='', anchor=tk.CENTER)
    Table.heading('SP', text='System Property', anchor=tk.CENTER)
    Table.heading('V', text='Value', anchor=tk.CENTER)

    Table.insert(parent='', index=0, iid=0, text='', values=("Architecture ",platform.machine()))
    pname=subprocess.run(["wmic","cpu","get","Name"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input="")
    out=pname.stdout.decode('UTF-8').split('\r\r\n')
    Table.insert(parent='', index=1, iid=1, text='', values=("Processor "+out[0].rstrip(),out[1]))
    Table.insert(parent='', index=2, iid=2, text='', values=("L1CacheSize ","256.0 KB"))
    l2=subprocess.run(["wmic","cpu","get","L2CacheSize"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input="")
    out=l2.stdout.decode('UTF-8').split('\r\r\n')
    Table.insert(parent='', index=3, iid=3, text='', values=((out[0].rstrip(),str(int(out[1])/(1024))+"MB")))
    l3=subprocess.run(["wmic","cpu","get","L3CacheSize"],stdout=subprocess.PIPE,stderr=subprocess.PIPE,input="")
    out=l3.stdout.decode('UTF-8').split('\r\r\n')
    Table.insert(parent='', index=4, iid=4, text='', values=((out[0].rstrip(),str(int(out[1])/(1024))+"MB")))
    Table.insert(parent='', index=5, iid=5, text='', values=("RAM ",str(round(psutil.virtual_memory().total / (1024.0 **3)))+" GB"))
    Table.insert(parent='', index=6, iid=6, text='', values=("Physical cores ", psutil.cpu_count(logical=False)))
    Table.insert(parent='', index=7, iid=7, text='', values=("Total cores ", psutil.cpu_count(logical=True)))
    Table.insert(parent='', index=8, iid=8, text='', values=("Max CPU Frequency ",str(psutil.cpu_freq().max)+"Mhz"))

    Table.pack(padx=20,pady=10)
    SelectFileButton.pack(padx=3,pady=3)

    root.title("C++ Program Slicer")
    root.protocol('WM_DELETE_WINDOW', lambda: Close(root))
    return root

if __name__=="__main__":
    main().mainloop()