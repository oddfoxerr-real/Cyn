// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

public final class CynConnectionStreakTest {
    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }

    private static void complete(CynConnectionStreak streak, long connectAtMs) {
        streak.onConnect(connectAtMs);
        check(!streak.onDisconnect(connectAtMs + 1000L), "Premature trigger");
    }

    private static void fourConsecutiveCyclesTriggerExactlyOnce() {
        CynConnectionStreak streak = new CynConnectionStreak();
        complete(streak, 1000L);
        complete(streak, 3000L);
        complete(streak, 5000L);
        streak.onConnect(7000L);
        check(streak.onDisconnect(8000L), "Fourth disconnect must trigger");
        complete(streak, 9000L);
        complete(streak, 11000L);
        complete(streak, 13000L);
        streak.onConnect(15000L);
        check(streak.onDisconnect(16000L), "A new four-cycle streak should trigger");
    }

    private static void twoNowTwoLaterNeverTrigger() {
        CynConnectionStreak streak = new CynConnectionStreak();
        complete(streak, 1000L);
        complete(streak, 3000L);
        complete(streak, 50000L);
        streak.onConnect(52000L);
        check(!streak.onDisconnect(53000L), "Old cycles must expire");
        streak.onConnect(54000L);
        check(!streak.onDisconnect(55000L), "Three fresh cycles not enough");
        streak.onConnect(56000L);
        check(streak.onDisconnect(57000L), "Four fresh cycles required");
    }

    private static void slowConnectDisconnectDoesNotCount() {
        CynConnectionStreak streak = new CynConnectionStreak();
        streak.onConnect(1000L);
        check(!streak.onDisconnect(22001L), "Late disconnect must reset streak");
        complete(streak, 23000L);
    }

    private static void disconnectWithoutConnectDoesNotCount() {
        CynConnectionStreak streak = new CynConnectionStreak();
        check(!streak.onDisconnect(1000L), "Disconnect alone cannot count");
        complete(streak, 2000L);
    }

    private static void duplicateConnectDoesNotAddExtraCycle() {
        CynConnectionStreak streak = new CynConnectionStreak();
        complete(streak, 1000L);
        streak.onConnect(3000L);
        streak.onConnect(4000L);
        check(!streak.onDisconnect(5000L), "Repeated Connect must not count twice");
        complete(streak, 6000L);
        complete(streak, 8000L);
        streak.onConnect(10000L);
        check(streak.onDisconnect(11000L), "Only four complete pairs since the reset may trigger");
    }

    private static void resetOnRecreation() {
        CynConnectionStreak first = new CynConnectionStreak();
        complete(first, 1000L);
        complete(first, 3000L);
        CynConnectionStreak next = new CynConnectionStreak();
        complete(next, 5000L);
        complete(next, 7000L);
        next.onConnect(9000L);
        check(!next.onDisconnect(10000L), "Streak must not persist across app sessions");
    }

    public static void main(String[] args) {
        fourConsecutiveCyclesTriggerExactlyOnce();
        twoNowTwoLaterNeverTrigger();
        slowConnectDisconnectDoesNotCount();
        disconnectWithoutConnectDoesNotCount();
        duplicateConnectDoesNotAddExtraCycle();
        resetOnRecreation();
        System.out.println("PASS: CYN in-app 4-cycle Easter egg detection");
    }
}
