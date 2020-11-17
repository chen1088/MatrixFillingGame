package com.applications;

import javax.swing.*;

public class WorkerStatCountOfBlank extends MFGSwingWorker {
   public WorkerStatCountOfBlank(BMContainer c) {
      super(c);
   }
   public WorkerStatCountOfBlank(BMContainer c, JTextPane tf, JProgressBar pb, JCheckBox cb,JPanel rp)
   {
      super(c,tf,pb,cb,rp);
   }
   @Override
   public MFGSwingWorker Copy() {
      return new WorkerStatCountOfBlank(container,resultField,progressBar,enable, subPanel);
   }

   @Override
   public void SetAppearance() {
      enable.setText("BlankStat");
   }

   @Override
   protected String doInBackground() throws Exception {
      Integer[][] maxconf = container.GetMaximalConf();
      int Bcount = 0;
      int B0count = 0;
      int h = container.Height();
      int w = container.Width();
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            if(maxconf[i][j] == 2)
               Bcount++;
      boolean[][] oneimpmap = new boolean[h][w];
      boolean[][] h1map = new boolean[w][w];
      boolean[][] h2map = new boolean[w][w];
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            for(int k = j+1;k<w;++k)
            {
               if(maxconf[i][j] == 2 && maxconf[i][k] == 0)
                  h1map[j][k] = true;
               else if(maxconf[i][j] == 0 && maxconf[i][k] == 2 && h1map[j][k])
                  oneimpmap[i][k] = true;
            }
      for(int i = h - 1;i>=0;--i)
         for(int j = 0;j<w;++j)
            for(int k = j+1;k<w;++k)
            {
               if(maxconf[i][j] == 0 && maxconf[i][k] == 2)
                  h2map[j][k] = true;
               else if(maxconf[i][j] == 2 && maxconf[i][k] == 0 && h2map[j][k])
                  oneimpmap[i][j] = true;
            }
      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
            if(oneimpmap[i][j])
               B0count++;
      return String.format("%d/%d",B0count,Bcount);
   }
}
