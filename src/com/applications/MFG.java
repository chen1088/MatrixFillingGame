package com.applications;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

public class MFG {
   public static void main(String[] args) {
      try{
         UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
      }catch (Exception e)
      {

      }
      fonts = GraphicsEnvironment.getLocalGraphicsEnvironment().getAvailableFontFamilyNames();
      redone = new Font(fonts[0],Font.BOLD,18);
      greenzero = new Font(fonts[0],Font.PLAIN, 18);
      MFG instance = new MFG();

   }
   private static String[] fonts;
   private static Font redone;
   private static Font greenzero;
   public MFG(){
      _buttonCache = new ArrayList<>();
      frame = new JFrame("MFG");
      frame.setContentPane(mainPanel);
      frame.addKeyListener(new KeyListener() {
         @Override
         public void keyTyped(KeyEvent e) {

         }

         @Override
         public void keyPressed(KeyEvent e) {

         }

         @Override
         public void keyReleased(KeyEvent e) {
            if(e.getKeyCode() == KeyEvent.VK_Z) {
               container.Revert();
               RefreshGridPanel();
            }
         }
      });
      frame.setFocusable(true);
      frame.requestFocusInWindow();
      frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      container = new BMContainer();
      compute.addActionListener(new ActionListener() {
         @Override
         public void actionPerformed(ActionEvent e) {
            computeAction();
         }
      });
      updateEveryStep.addItemListener(new ItemListener() {
         @Override
         public void itemStateChanged(ItemEvent e) {
            compute.setEnabled(!compute.isEnabled());
         }
      });
      // stats panel
      worker = new WorkerStatCountM(container,result1,progressBar1);
      RefreshGridPanel();
      frame.setVisible(true);
   }
   private AtomicBoolean atomic = new AtomicBoolean(false);
   private SwingWorker worker;
   private Thread workingthread;
   private void computeAction() {
      worker.cancel(true);
      worker = new WorkerStatCountM(container,result1,progressBar1);
      worker.execute();
//      worker = new SwingWorker() {
//         @Override
//         protected Object doInBackground() throws Exception {
//            return null;
//         }
//      };

//      if(workingthread != null&&workingthread.isAlive())
//      {
//         workingthread.interrupt();
////         atomic.set(false);
//      }
////      atomic.set(true);
//      workingthread = new Thread(){
//         public void run(){
//            double r = 0.0;
//            double r2;
//            double r3;
//            long r4 = 0;
//            try{
//               if(ratio.isSelected())
//                  r = container.RatioDRFfillingsOfMaxConf();
//               if(exact.isSelected())
//                  r4 = container.CountDRFfillingsOfMaxConf();
//               r2 = container.RatioDRFfillings();
//               r3 = container.APRRatioDRFfillings();
//            }catch(InterruptedException e)
//            {
//               return;
//            }
//            StringBuilder resultString = new StringBuilder();
//            if(ratio.isSelected())
//               resultString.append(String.format("RatioOfMax:%.10f\n",r));
//            if(exact.isSelected())
//               resultString.append(String.format("ExactCountOfMax:%d\n",r4));
//            resultString.append(String.format("RatioOfAll1:%.10f\n",r2));
//            resultString.append(String.format("APRRatioOfAll1:%.10f",r3));
//            result.setText(resultString.toString());// r2 should be small
////            atomic.set(false);
//         }
//      };
      result.setText("Computing");
//      workingthread.start();
   }

   private JFrame frame;
   private JPanel mainPanel;
   private JPanel gridPanel;
   private JPanel rightPanel;
   private JPanel downPanel;
   private JRadioButton ratio;
   private JRadioButton exact;
   private JCheckBox updateEveryStep;
   private JTextArea result;
   private JButton compute;
   private JCheckBox enable1;
   private JTextField result1;
   private JProgressBar progressBar1;
   private JPanel worker1;
   private BMContainer container;
   private ArrayList<JButton> _buttonCache;

   private void RefreshGridPanel()
   {
      RefreshMatPanel(gridPanel);
      if(updateEveryStep.isSelected())
         computeAction();
      frame.pack();
   }
   private void MakeStatsPanel()
   {

   }
   private void RefreshMatPanel(JPanel pane)
   {
      _buttonCache.clear();
      pane.removeAll();
      pane.setLayout(new GridBagLayout());
      GridBagConstraints gbc = new GridBagConstraints();
      Integer[][] m = container.GetMaximalConf();
      final int panelh = 500;
      final int panelw = 500;
      final int maxw = 50;
      final int maxh = 50;
      // side bars
      int h = container.Height();
      int w = container.Width();
      int hs = Math.min(maxh, (panelh-50)/(2*h+3));
      int ws = Math.min(maxw, (panelw-50)/(2*w+3));
      int rn = 2 * h + 2;
      int cn = 2 * w + 2;
      gbc.gridx = 0;
      gbc.ipadx = 50;
      gbc.ipady = hs;
      for(int i = 0;i<h+1;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(50,hs));
         b.setBackground(Color.red);
         b.setOpaque(true);
         b.setText("+");
         b.setMargin(new Insets(0, 0, 0, 0));
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               JButton eb = (JButton)e.getSource();
               container.InsertRow(Integer.parseInt(eb.getName()));
               RefreshGridPanel();
            }
         });
         gbc.gridy = 2*i+1;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      for(int i = 0;i<h;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(50,hs));
         b.setBackground(Color.gray);
         b.setOpaque(true);
         b.setText("-");
         b.setMargin(new Insets(0, 0, 0, 0));
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               if(container.Height()<=1)return;
               JButton eb = (JButton)e.getSource();
               container.RemoveRow(Integer.parseInt(eb.getName()));
               RefreshGridPanel();
            }
         });
         gbc.gridy = 2*i+2;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      gbc.gridy = 0;
      gbc.ipadx = ws;
      gbc.ipady = 50;
      for(int i = 0;i<w+1;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(ws,50));
         b.setBackground(Color.red);
         b.setOpaque(true);
         b.setText("+");
         b.setMargin(new Insets(0, 0, 0, 0));
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               JButton eb = (JButton)e.getSource();
               container.InsertCol(Integer.parseInt(eb.getName()));
               RefreshGridPanel();
            }
         });
         gbc.gridx = 2*i+1;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      for(int i = 0;i<w;++i)
      {
         JButton b = new JButton();
         b.setPreferredSize(new Dimension(ws,50));
         b.setBackground(Color.gray);
         b.setOpaque(true);
         b.setText("-");
         b.setMargin(new Insets(0, 0, 0, 0));
         b.setName(String.valueOf(i));
         b.setFocusable(false);
         b.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
               if(container.Width()<=1)return;
               JButton eb = (JButton)e.getSource();
               container.RemoveCol(Integer.parseInt(eb.getName()));
               RefreshGridPanel();
            }
         });
         gbc.gridx = 2*i+2;
         _buttonCache.add(b);
         pane.add(b,gbc);
      }
      //matrix
      gbc.ipadx = ws;
      gbc.ipady = hs;

      for(int i = 0;i<h;++i)
         for(int j = 0;j<w;++j)
         {
            JButton b = new JButton();
            b.setPreferredSize(new Dimension(ws,hs));
            b.setMargin(new Insets(0, 0, 0, 0));
            b.setName(Integer.toString(i)+","+Integer.toString(j));
            b.setFocusable(false);
            gbc.gridx = 2*j+2;
            gbc.gridy = 2*i+2;
            if(m[i][j] == 1)
            {
               b.setText("1");
               b.setFont(redone);
               b.setForeground(Color.red);
               b.setBackground(Color.red);
               b.addActionListener(new ActionListener() {
                  @Override
                  public void actionPerformed(ActionEvent e) {
                     JButton eb = (JButton)e.getSource();
                     String[] ebs = eb.getName().split(",");
                     Integer r = Integer.parseInt(ebs[0]);
                     Integer c = Integer.parseInt(ebs[1]);
                     container.SetEntry(r,c,2);
                     RefreshGridPanel();
                  }
               });
            }
            else if(m[i][j] == 0)
            {
               b.setText("0");
               b.setFont(greenzero);
               b.setForeground(Color.BLUE);
               b.setBackground(Color.BLUE);
               b.addActionListener(new ActionListener() {
                  @Override
                  public void actionPerformed(ActionEvent e) {
                     JButton eb = (JButton)e.getSource();
                     String[] ebs = eb.getName().split(",");
                     Integer r = Integer.parseInt(ebs[0]);
                     Integer c = Integer.parseInt(ebs[1]);
                     container.SetEntry(r,c,1);
                     RefreshGridPanel();
                  }
               });
            }
            else if(m[i][j]==2)
            {
               b.addActionListener(new ActionListener() {
                  @Override
                  public void actionPerformed(ActionEvent e) {
                     JButton eb = (JButton)e.getSource();
                     String[] ebs = eb.getName().split(",");
                     Integer r = Integer.parseInt(ebs[0]);
                     Integer c = Integer.parseInt(ebs[1]);
                     container.SetEntry(r,c,1);
                     RefreshGridPanel();
                  }
               });
            }
            _buttonCache.add(b);
            pane.add(b,gbc);
         }
      //pane.revalidate();
      pane.repaint();
   }

}
