// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.content.Context;
import android.util.AtomicFile;
import org.json.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

final class LearningStore {
    final ArrayList<PreferenceModel.Example> examples=new ArrayList<>();
    PreferenceModel model=new PreferenceModel();
    String message="Personal model: add More and Less examples, then train.";
    private final Context context;
    LearningStore(Context context) { this.context=context; load(); }
    private static double[] numbers(JSONArray a, int n) throws JSONException {
        if(a.length()!=n) throw new JSONException("Wrong dimensions"); double[] out=new double[n];
        for(int i=0;i<n;i++) { out[i]=a.getDouble(i); if(!Double.isFinite(out[i])) throw new JSONException("Non-finite weight"); } return out;
    }
    private void readModel(JSONObject j) throws JSONException {
        double[] w=numbers(j.getJSONArray("weights"),209); PreferenceModel m=new PreferenceModel();
        System.arraycopy(w,0,m.weights,0,209); JSONArray r=j.getJSONArray("ready");
        for(int i=0;i<5;i++) m.ready[i]=r.optBoolean(i,false); model=m;
    }
    void starter() throws Exception {
        try(InputStream in=context.getAssets().open("cyn-starter.json")) {
            JSONObject j=new JSONObject(Streams.text(in));
            readModel(j); model.ready[0]=true; model.ready[4]=true;
        }
        message="Experimental desktop starter loaded; Android sound analysis differs."; save();
    }
    private void load() {
        AtomicFile file=new AtomicFile(new File(context.getFilesDir(),"learning.json"));
        try(FileInputStream in=file.openRead()) {
            JSONObject j=new JSONObject(Streams.text(in)); readModel(j);
            JSONArray a=j.optJSONArray("examples");
            if(a!=null) for(int i=Math.max(0,a.length()-512);i<a.length();i++) { JSONObject e=a.getJSONObject(i); int o=e.getInt("effect"); if(o>=0&&o<5) examples.add(new PreferenceModel.Example(numbers(e.getJSONArray("x"),11),o,e.getBoolean("more"),e.getLong("id"))); }
            message="Personal model and feedback restored on this phone.";
        } catch(FileNotFoundException ignored) { } catch(Exception e) { message="Saved learning data could not be read; using a fresh model."; }
    }
    void save() throws Exception {
        JSONObject j=new JSONObject(); j.put("schema",1); j.put("weights",new JSONArray(model.weights)); j.put("ready",new JSONArray(model.ready));
        JSONArray a=new JSONArray(); for(PreferenceModel.Example e:examples) { JSONObject v=new JSONObject(); v.put("x",new JSONArray(e.x)); v.put("effect",e.effect); v.put("more",e.more); v.put("id",e.id); a.put(v); } j.put("examples",a);
        AtomicFile f=new AtomicFile(new File(context.getFilesDir(),"learning.json")); FileOutputStream out=null;
        try { out=f.startWrite(); out.write(j.toString().getBytes(StandardCharsets.UTF_8)); f.finishWrite(out); } catch(Exception e) { if(out!=null) f.failWrite(out); throw e; }
    }
    void label(List<AudioService.Frame> frames, int effect, boolean more) throws Exception {
        for(AudioService.Frame frame:frames) {
            examples.removeIf(e->e.id==frame.id&&e.effect==effect);
            examples.add(new PreferenceModel.Example(frame.x,effect,more,frame.id));
        }
        while(examples.size()>512) examples.remove(0); save();
    }
    void reset() throws Exception { model=new PreferenceModel(); examples.clear(); message="Personal model reset."; save(); }
}
