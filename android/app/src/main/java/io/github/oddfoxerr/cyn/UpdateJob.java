// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.app.job.*;
public final class UpdateJob extends JobService {
    private volatile boolean stopped;
    @Override public boolean onStartJob(JobParameters params){stopped=false;UpdateManager.check(this,false,()->{if(!stopped)jobFinished(params,false);});return true;}
    @Override public boolean onStopJob(JobParameters params){stopped=true;return true;}
}
