package fr.cerostudio.api.window;

public final class WindowApi {

    private volatile String overrideTitle;

    public void setTitle(String title) {
        this.overrideTitle = title;
    }

    public String getTitle() {
        return overrideTitle;
    }

    public String resolve(String originalTitle) {
        return overrideTitle != null ? overrideTitle : originalTitle;
    }
}
