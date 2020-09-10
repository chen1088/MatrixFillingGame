package com.applications;

import javax.swing.*;
import java.awt.*;

public class WorkerStatCNFStr extends MFGSwingWorker {
   public WorkerStatCNFStr(BMContainer c) {
      super(c);
   }
   public WorkerStatCNFStr(BMContainer c, JTextField tf, JProgressBar pb, JCheckBox cb)
   {
      super(c,tf,pb,cb);
   }
   @Override
   public MFGSwingWorker Copy() {
      return new WorkerStatCNFStr(container,resultField,progressBar,enable);
   }

   @Override
   public void SetAppearance() {
      enable.setText("CNFStr");
      resultField.setPreferredSize(new Dimension(200,500));
      progressBar.setVisible(false);
   }

   @Override
   protected String doInBackground() throws Exception {
      Integer[][] maxconf = container.GetMaximalConf();
      StringBuilder b = new StringBuilder();
      int Bcount = 1;
      int B0count = 1;
      int h = container.Height();
      int w = container.Width();
      FourDNF dnf = container.ConvertToDNF(new boolean[h][w],maxconf);

      //TODO: Not implemented
      return "";
   }
}
