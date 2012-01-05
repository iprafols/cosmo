// Compares two power spectra generated by the --save-transfer option to cosmocalc.

TCanvas *canvas;

void plotPower(const char *xfer0Name = "xfer0.dat", const char *xfer1Name = "xfer1.dat") {
    // Initialize graphics options
    gROOT->SetStyle("Plain");
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    
    canvas = new TCanvas("canvas","canvas",800,600);
    //canvas->SetLeftMargin(0.13);
    canvas->SetLogx();
    canvas->SetLogy();
    canvas->SetGridx();
    canvas->SetGridy();
    
    TGraph *g0 = new TGraph(xfer0Name,"%lg %*lg %lg %*lg %*lg %*lg");
    g0->SetLineWidth(4);
    g0->SetLineColor(kBlue-8);
    g0->Draw("AC");
    g0->GetHistogram()->SetXTitle("Wavenumber k (h/Mpc)");
    g0->GetHistogram()->SetYTitle("Power Spectrum (4#pi^{3}/k^{3}) P(k)");
    TGraph *g1 = new TGraph(xfer1Name,"%lg %*lg %lg %*lg %*lg %*lg");
    g1->SetLineWidth(4);
    g1->SetLineStyle(kDashed);
    g1->SetLineColor(kBlue-8);
    g1->Draw("C");
}
