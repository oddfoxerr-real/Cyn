import unittest
import torch
from apex.model import Apex6M, count_parameters, forward_audio
from apex.data import example
from apex.train import si_sdr

class ApexTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        torch.set_num_threads(2)
        cls.net=Apex6M().eval()

    def test_parameter_capacity(self):
        self.assertGreaterEqual(count_parameters(self.net),5_000_000)
        self.assertLess(count_parameters(self.net),8_000_000)

    def test_waveform_roundtrip_shape(self):
        audio,clean,scene=example(123)
        with torch.no_grad():
            y,logits,mask=forward_audio(self.net,audio[None])
        self.assertEqual(y.shape,(1,48000))
        self.assertEqual(logits.shape,(1,6))
        self.assertTrue(torch.isfinite(y).all().item())
        self.assertTrue(((mask>=0)&(mask<=1)).all().item())

    def test_data_repeatable(self):
        x,y,l=example(345)
        a,b,c=example(345)
        self.assertTrue(torch.equal(x,a))
        self.assertTrue(torch.equal(y,b))
        self.assertTrue(torch.equal(l,c))

    def test_sdr(self):
        target=torch.randn(1,48000)
        noisy=target+0.3*torch.randn(1,48000)
        self.assertGreater(float(si_sdr(target,target)),float(si_sdr(noisy,target)))

if __name__=='__main__':unittest.main()