package gui;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;

public class FADisplayPanel {
   private JPanel panel1;
   private JLabel pic_label;
   private JButton show_image_button;

   public static void main(String[] args) {

      JFrame frame = new JFrame("DFADisplay");
      frame.setContentPane(new FADisplayPanel().panel1);
      frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      frame.pack();
      frame.setVisible(true);
   }
   public FADisplayPanel() {
      show_image_button.addActionListener(new ActionListener() {
         @Override
         public void actionPerformed(ActionEvent e) {
            pic_label.setIcon(new ImageIcon(TestGraphviz.gettestimage()));
         }
      });
   }
}
