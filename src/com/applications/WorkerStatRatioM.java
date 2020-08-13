package com.applications;

import javax.swing.*;

public class WorkerStatRatioM extends MFGSwingWorker{

   public WorkerStatRatioM(BMContainer c) {
      super(c);
   }

   @Override
   public MFGSwingWorker Copy() {
      WorkerStatRatioM ret = new WorkerStatRatioM(container);
      ret.resultField = resultField;
      ret.enable = enable;
      ret.progressBar = progressBar;
      return ret;
   }

   @Override
   public void SetAppearance() {
      enable.setText("RatioOfM");
   }

   @Override
   protected String doInBackground() {
      boolean[][] ignm = container.GetFullIgnoreMap();
      Integer[][] m = container.GetMaximalConf();
      int numberofvars = 0;
      int h = container.Height();
      int w = container.Width();
      for (int i = 0; i < h; ++i)
         for (int j = 0; j < w; ++j)
            if (m[i][j] == 2)
               numberofvars++;
      if (numberofvars >= 40)
         return "-1.0";
      FourDNF dnf = container.ConvertToDNF(ignm, m);
      dnf.caller = this;
      long tmppower = 1;
      for (int i = 0; i < numberofvars; ++i)
         tmppower *= 2;
      return String.format("%.10f",(double)(tmppower - dnf.Count())/(double)tmppower);
   }
}
