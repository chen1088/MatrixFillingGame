package com.applications;

import javax.swing.*;
import java.awt.*;
import java.util.concurrent.ExecutionException;

public class WorkerStatCNFStr extends MFGSwingWorker {
   public JScrollPane scrollPane;
   public JTextPane result2;
   public FourDNF dnf;
   public WorkerStatCNFStr(BMContainer c) {
      super(c);
   }
   public WorkerStatCNFStr(BMContainer c, JTextPane tf, JProgressBar pb, JCheckBox cb,JPanel rp)
   {
      super(c,tf,pb,cb,rp);
   }
   @Override
   public MFGSwingWorker Copy() {
      return new WorkerStatCNFStr(container,resultField,progressBar,enable, subPanel);
   }

   @Override
   public void SetAppearance() {
      enable.setText("CNFStr");
      //resultField.setVisible(false);
      result2 = new JTextPane();
      scrollPane = new JScrollPane();
      //result2.setPreferredSize(new Dimension(200,500));
      scrollPane.setPreferredSize(new Dimension(200,500));

      subPanel.add(scrollPane);
      scrollPane.add(result2);
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
      dnf = container.ConvertToDNF(new boolean[h][w],maxconf);
      return "";
   }

   @Override
   protected void done()
   {
      if(!isCancelled())
         if(resultField != null)
            dnf.ToTextPane(resultField);
      super.done();
   }
}
